#include "pch.h"
#include <cstdint>
#include <vector>
#include <string>
#include <sstream>
#include <detours/detours.h>
#include "../Injector/helper.hpp"
#include "driver.hpp"

constexpr int debug_ =
#ifdef _DEBUG
1;
#else
0;
#endif

static HANDLE keyboard_device;

//Return 0 if succeeds.
extern "C" __declspec(dllexport) uint64_t __stdcall IbLogiInit() {
    HANDLE pipe = CreateFileW(
        LR"(\\.\pipe\{B887DC25-1FF4-4409-95B9-A94EB4AAA3D6})",
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        0
    );
    if (pipe == INVALID_HANDLE_VALUE)
        //#TODO: start LGS?
        return ((uint64_t)GetLastError() << 32) + 1;
    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(pipe, &mode, nullptr, nullptr);  //#TODO: necessary?

    struct Request {
        uint32_t opcode;
    } request;
    request.opcode = 1;
    DWORD written;
    if (!WriteFile(pipe, &request, sizeof Request, &written, nullptr)) {
        CloseHandle(pipe);
        return ((uint64_t)GetLastError() << 32) + 2;
    }

    struct Response {
        uint64_t error;
        uint64_t data;
    } response;
    DWORD bytes_read;
    if (!ReadFile(pipe, &response, sizeof Response, &bytes_read, nullptr)) {
        CloseHandle(pipe);
        return ((uint64_t)GetLastError() << 32) + 3;
    }
    DebugOutput(wstringstream() << L"LogiLib: error  " << response.error << L", " << response.data);
    if (response.error) {
        CloseHandle(pipe);
        return ((uint64_t)GetLastError() << 32) + 4;
    }

    keyboard_device = (HANDLE)response.data;
    return 0;

    /*
    if (CallNamedPipeW(
        LR"(\\.\pipe\{B887DC25-1FF4-4409-95B9-A94EB4AAA3D6})",
        &request,
        sizeof Request,
        &result,
        sizeof(result),
        &bytes_read,
        NMPWAIT_NOWAIT
    )) {
        DebugOutput(wstringstream() << L"LogiLib: result " << result << L", " << (uintptr_t)keyboard_device);
        return result;
    }
    else {
        return GetLastError();
    }
    */
}

extern "C" __declspec(dllexport) void __stdcall IbLogiDestory() {
    CloseHandle(keyboard_device);
}


static decltype(SendInput)* SendInputTrue = SendInput;
extern "C" __declspec(dllexport) UINT WINAPI IbLogiSendInput(UINT cInputs, LPINPUT pInputs, int cbSize) {
    /*if (debug_) {
        std::wstringstream ss;
        ss << L"IbLogiLib.SendInput: ";
        for (UINT i = 0; i < cInputs; i++) {
            ss << (int)pInputs[i].ki.wVk << " " << (pInputs[i].ki.dwFlags & KEYEVENTF_KEYUP ? 0 : 1) << ", ";
        }
        OutputDebugStringW(ss.str().c_str());
    }*/

    DriverKeyboardSend(keyboard_device, pInputs, cInputs);
    return cInputs;
}

//#TODO: only needed when two AHK processes exist?
static decltype(GetAsyncKeyState)* GetAsyncKeyStateTrue = GetAsyncKeyState;
SHORT WINAPI GetAsyncKeyStateDetour(int vKey) {
    DebugOutput(wstringstream() << L"LogiLib.GetAsyncKeyState: " << vKey << ", " << DriverGetAsyncKeyState(vKey, GetAsyncKeyStateTrue));
    return DriverGetAsyncKeyState(vKey, GetAsyncKeyStateTrue);
}

extern "C" __declspec(dllexport) bool __stdcall IbLogiSendInputHookBegin() {
    if (!keyboard_device)
        return false;
    DriverSyncKeyStates();
    IbDetourAttach(&GetAsyncKeyStateTrue, GetAsyncKeyStateDetour);
    IbDetourAttach(&SendInputTrue, IbLogiSendInput);
    return true;
}
extern "C" __declspec(dllexport) void __stdcall IbLogiSendInputHookEnd() {
    IbDetourDetach(&SendInputTrue, IbLogiSendInput);
    IbDetourDetach(&GetAsyncKeyStateTrue, GetAsyncKeyStateDetour);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    DebugOutput(L"LogiLib.DllMain: " + std::to_wstring(ul_reason_for_call));
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        //don't auto init so that incorrect usage can be exposed
        //#TODO: or lock self's ref count?
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        IbLogiDestory();
        break;
    }
    return TRUE;
}