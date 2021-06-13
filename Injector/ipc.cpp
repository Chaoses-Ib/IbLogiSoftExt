#include "pch.h"
#include "ipc.hpp"
#include <vector>
#include <thread>
#include <Windows.h>
#include "helper.hpp"

using namespace ib;

const DWORD buffer_size = 16;

struct RoutineData {
    OVERLAPPED overlap;
    HANDLE pipe;
    Byte in_buf[buffer_size];
    Byte out_buf[buffer_size];
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
                buffer_size,
                buffer_size,
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
        if (!ReadFileEx(data->pipe, data->in_buf, sizeof(data->in_buf), &data->overlap, IpcReadRoutine))
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

    struct Request {
        uint32_t opcode;
    } *request = auto_cast(data->in_buf);

    struct Response {
        uint64_t error;
        uint64_t data;
    } *response = auto_cast(data->out_buf);
    
    response->error = [data, request, response]() -> uint64_t {
        if (request->opcode == 1 || request->opcode == 2) {
            ULONG client_pid;
            GetNamedPipeClientProcessId(data->pipe, &client_pid);
            HANDLE client = OpenProcess(PROCESS_DUP_HANDLE, false, client_pid);
            if (!client) {
                return ((uint64_t)GetLastError() << 32) + 2;
            }

            Module LCore = *ModuleFactory::CurrentProcess();
            if (request->opcode == 1) {
                HANDLE keyboard_device = LCore.base[0x10B8220][0x10][0x8];
                bool success = DuplicateHandle(GetCurrentProcess(), keyboard_device, client, (HANDLE*)&response->data, 0, false, DUPLICATE_SAME_ACCESS);  //#TODO: cut down access
                return success ? 0 : ((uint64_t)GetLastError() << 32) + 3;
            }
            else {  //#TODO
                return 1;
            }
        }
        else {
            return 1;
        }
    }();

    if (!WriteFileEx(data->pipe, data->out_buf, sizeof(data->out_buf), &data->overlap, IpcWriteRoutine))
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