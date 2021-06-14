#include "pch.h"
#include "ipc.hpp"
#include <vector>
#include <thread>
#include <Windows.h>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include "helper.hpp"

using namespace ib;

struct RoutineData {
    OVERLAPPED overlap;
    HANDLE pipe;
    Request request;
    Response response;
};

void WINAPI IpcReadRoutine(DWORD error, DWORD size, OVERLAPPED* overlap);

void IpcStart() {
    std::thread t([] {
        OVERLAPPED connect{};  //should be initialized to zero
        //"if you use an auto-reset event, your application can stop responding if you wait for the operation to complete and then call GetOverlappedResult with the bWait parameter set to TRUE."
        connect.hEvent = CreateEventW(nullptr, false, true, nullptr);
        if (!connect.hEvent)
            return;
        bool pending = false;

        while (true) {
            HANDLE pipe = CreateNamedPipeW(
                LR"(\\.\pipe\{B887DC25-1FF4-4409-95B9-A94EB4AAA3D6})",
                PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
                PIPE_UNLIMITED_INSTANCES,
                sizeof Response,
                sizeof Request,
                0,  //50ms
                nullptr);
            DebugOutput(L"CreatePipe: " + to_wstring((uintptr_t)pipe));

            ConnectNamedPipe(pipe, &connect);
            switch (GetLastError()) {
            case ERROR_IO_PENDING:
                pending = true;
                break;
            case ERROR_PIPE_CONNECTED:
                SetEvent(connect.hEvent);
                break;
            }

            bool connected = false;
            while (!connected) {
                switch (WaitForSingleObjectEx(connect.hEvent, INFINITE, true)) {
                case WAIT_OBJECT_0:
                {
                    DebugOutput(L"ConnectPipe");
                    if (pending) {
                        DWORD size;
                        GetOverlappedResult(pipe, &connect, &size, false);
                    }

                    RoutineData* data = new RoutineData;
                    data->pipe = pipe;
                    data->overlap = OVERLAPPED();  //should be initialized to zero
                    IpcReadRoutine(0, 0, auto_cast(data));

                    connected = true;
                    break;
                }
                case WAIT_IO_COMPLETION:
                    continue;
                }
            }
        }
        });
    t.detach();
}

void WINAPI IpcWriteRoutine(DWORD error, DWORD size, OVERLAPPED* overlap);

void WINAPI IpcReadRoutine(DWORD error, DWORD size, OVERLAPPED* overlap)
{
    DebugOutput(wstringstream() << L"ReadRoutine: " << error << L", " << size);
    RoutineData* data = auto_cast(overlap);
    auto close = [&data] {
        FlushFileBuffers(data->pipe);
        DisconnectNamedPipe(data->pipe);
        CloseHandle(data->pipe);
        delete data;
    };
    auto read = [&data, &close] {
        if (!ReadFileEx(data->pipe, &data->request, sizeof(data->request), &data->overlap, IpcReadRoutine))
            close();
    };
    if (!error && !size) {  //call from IpcStart
        read();
        return;
    }
    if (error) {
        close();
        return;
    }

    data->response.error = [data]() -> uint64_t {
        if (data->request.opcode == 1) {
            ULONG client_pid;
            GetNamedPipeClientProcessId(data->pipe, &client_pid);
            HANDLE client = OpenProcess(PROCESS_DUP_HANDLE, false, client_pid);
            if (!client) {
                return ((uint64_t)GetLastError() << 32) + 2;
            }

            /*
            <?xml version="1.0" encoding="utf-8"?>
            <CheatTable>
              <CheatEntries>
                <CheatEntry>
                  <ID>0</ID>
                  <Description>"Device handle"</Description>
                  <LastState Value="000004A0" RealAddress="1FFD6F0FF78"/>
                  <ShowAsHex>1</ShowAsHex>
                  <ShowAsSigned>0</ShowAsSigned>
                  <VariableType>4 Bytes</VariableType>
                  <Address>LCore.exe+10B8220</Address>
                  <Offsets>
                    <Offset>8</Offset>
                    <Offset>10</Offset>
                  </Offsets>
                </CheatEntry>
              </CheatEntries>
            </CheatTable>
            */
            Module LCore = *ModuleFactory::CurrentProcess();
            HANDLE device = LCore.base[0x10B8220][0x10][0x8];
            bool success = DuplicateHandle(GetCurrentProcess(), device, client, (HANDLE*)&data->response.op1.device, 0, false, DUPLICATE_SAME_ACCESS);  //#TODO: cut down access
            if (!success) {
                return ((uint64_t)GetLastError() << 32) + 3;
            }

            data->response.op1.flags = 0;
            do {
                using namespace rapidjson;
                //#TODO: 20 warnings, wow...maybe getline is better? or disable warning for rapidjson?

                //fopen doesn't support environment variables
                wchar path[MAX_PATH];
                ExpandEnvironmentStringsW(LR"(%LOCALAPPDATA%\Logitech\Logitech Gaming Software\settings.json)", path, std::size(path));
                FILE* file = _wfsopen(path, L"rb", _SH_DENYNO);
                if (!file) {
                    DebugOutput(L"response.op1.flags: Opening file failed");
                    break;
                }
                char* buf = new char[16 * 1024];
                FileReadStream is(file, buf, sizeof(buf));

                Document doc;
                doc.ParseStream(is);
                
                //pointer.hasAcceleration
                //#TODO: FindMember
                if (doc.HasMember("pointer")) {
                    auto& pointer = doc["pointer"];
                    if (pointer.HasMember("hasAcceleration") && pointer["hasAcceleration"].GetBool()) {
                        data->response.op1.flags |= 1;
                    }
                    else {
                        DebugOutput(L"response.op1.flags: hasAcceleration not found");
                    }
                }
                else {
                    DebugOutput(L"response.op1.flags: pointer not found");
                }

                fclose(file);
                delete[] buf;
            } while (false);
            DebugOutput(L"response.op1.flags: " + to_wstring(data->response.op1.flags));

            return 0;
        }
        else {
            return 1;
        }
    }();

    if (!WriteFileEx(data->pipe, &data->response, sizeof(data->response), &data->overlap, IpcWriteRoutine))
        close();
}

void WINAPI IpcWriteRoutine(DWORD error, DWORD size, OVERLAPPED* overlap) {
    RoutineData* data = auto_cast(overlap);
    auto close = [&data] {
        FlushFileBuffers(data->pipe);
        DisconnectNamedPipe(data->pipe);
        CloseHandle(data->pipe);
        delete data;
    };
    close();
}