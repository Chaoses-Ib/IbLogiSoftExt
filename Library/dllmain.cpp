#include "pch.h"
#include <cstdint>
#include <vector>
#include <string>
#include <sstream>
#include <detours/detours.h>
#include "../Injector/helper.hpp"
#include "../Injector/ipc.hpp"
#include "driver.hpp"

constexpr int debug_ =
#ifdef _DEBUG
1;
#else
0;
#endif

static bool hook = false;

static decltype(SendInput)* SendInputTrue = SendInput;
extern "C" __declspec(dllexport) UINT WINAPI IbLogiSendInput(UINT cInputs, LPINPUT pInputs, int cbSize) {
    if (!hook)
        return SendInputTrue(cInputs, pInputs, cbSize);

    /*if (debug_) {
        std::wstringstream ss;
        ss << L"IbLogiLib.SendInput: ";
        for (UINT i = 0; i < cInputs; i++) {
            ss << (int)pInputs[i].ki.wVk << " " << (pInputs[i].ki.dwFlags & KEYEVENTF_KEYUP ? 0 : 1) << ", ";
        }
        OutputDebugStringW(ss.str().c_str());
    }*/

    DriverSend(pInputs, cInputs);
    return cInputs;
}

//#TODO: only needed when two AHK processes exist?
static decltype(GetAsyncKeyState)* GetAsyncKeyStateTrue = GetAsyncKeyState;
SHORT WINAPI GetAsyncKeyStateDetour(int vKey) {
    if (!hook)
        return GetAsyncKeyStateTrue(vKey);

    DebugOutput(wstringstream() << L"LogiLib.GetAsyncKeyState: " << vKey << ", " << DriverGetKeyState(vKey, GetAsyncKeyStateTrue));
    return DriverGetKeyState(vKey, GetAsyncKeyStateTrue);
}

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

    Request request;
    request.opcode = 1;
    DWORD written;
    if (!WriteFile(pipe, &request, sizeof Request, &written, nullptr)) {
        CloseHandle(pipe);
        return ((uint64_t)GetLastError() << 32) + 2;
    }

    Response response;
    DWORD bytes_read;
    if (!ReadFile(pipe, &response, sizeof Response, &bytes_read, nullptr)) {
        CloseHandle(pipe);
        return ((uint64_t)GetLastError() << 32) + 3;
    }
    DebugOutput(wstringstream() << L"LogiLib: error  " << response.error << L", " << response.op1.device);
    if (response.error) {
        CloseHandle(pipe);
        return ((uint64_t)GetLastError() << 32) + 4;
    }

    device = (HANDLE)response.op1.device;
    has_acceleration = response.op1.flags & 1;

    IbDetourAttach(&GetAsyncKeyStateTrue, GetAsyncKeyStateDetour);
    IbDetourAttach(&SendInputTrue, IbLogiSendInput);

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
        DebugOutput(wstringstream() << L"LogiLib: result " << result << L", " << (uintptr_t)device);
        return result;
    }
    else {
        return GetLastError();
    }
    */
}

extern "C" __declspec(dllexport) void __stdcall IbLogiDestroy() {
    IbDetourDetach(&SendInputTrue, IbLogiSendInput);
    IbDetourDetach(&GetAsyncKeyStateTrue, GetAsyncKeyStateDetour);
    CloseHandle(device);
}

extern "C" __declspec(dllexport) bool __stdcall IbLogiSendInputHookBegin() {
    if (!device)
        return false;
    DriverSyncKeyStates();
    hook = true;
    return true;
}
extern "C" __declspec(dllexport) void __stdcall IbLogiSendInputHookEnd() {
    hook = false;
}

BOOL APIENTRY DllMain(HMODULE hModule,
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
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
        //remember to break, or it will destroy
    case DLL_PROCESS_DETACH:
        IbLogiDestroy();
        break;
    }
    return TRUE;
}