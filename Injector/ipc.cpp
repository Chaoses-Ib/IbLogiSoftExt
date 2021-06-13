#include "pch.h"
#include "ipc.hpp"
#include <vector>
#include <thread>
#include <Windows.h>
#include "helper.hpp"
#include "driver.hpp"

using namespace ib;

const DWORD buffer_size = 256;

struct RoutineData {
    OVERLAPPED overlap;
    HANDLE pipe;
    Byte buf[buffer_size];
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
                LR"(\\.\pipe\{B887DC25-1FF4-4409-95B9-A94EB4AAA3D6}.Keyboard)",
                PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                PIPE_TYPE_BYTE,
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

void WINAPI IpcReadRoutine(DWORD error, DWORD size, OVERLAPPED* overlap)
{
    DebugOutput(wstringstream() << L"ReadRoutine: " << error << L", " << size);
    RoutineData* data = auto_cast(overlap);
    auto close = [&data] {
        DisconnectNamedPipe(data->pipe);
        CloseHandle(data->pipe);
        delete data;
    };
    auto read = [&data, &close] {
        if (!ReadFileEx(data->pipe, data->buf, sizeof(data->buf), &data->overlap, auto_cast(IpcReadRoutine)))
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
    
    //DebugOutput((const wchar*)(data->buf));
    DriverKeyboardSend((KeyboardInput*)data->buf, size / sizeof KeyboardInput);
    
    read();
}