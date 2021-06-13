#include "pch.h"
#include <cstdint>
#include <vector>
#include <string>
#include <sstream>
#include <detours/detours.h>

constexpr int debug_ =
#ifdef _DEBUG
1;
#else
0;
#endif

template<typename T>
LONG IbDetourAttach(_Inout_ T* ppPointer, _In_ T pDetour) {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach((void**)ppPointer, pDetour);
    return DetourTransactionCommit();
}

template<typename T>
LONG IbDetourDetach(_Inout_ T* ppPointer, _In_ T pDetour) {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach((void**)ppPointer, pDetour);
    return DetourTransactionCommit();
}

struct KeyboardInput {
    uint8_t vk;
    uint8_t flags;  //down = 1

    KeyboardInput(uint8_t vk, uint8_t flags) : vk(vk), flags(flags) {}
    KeyboardInput() : KeyboardInput(0, 0) {}
};

extern "C" __declspec(dllexport) int __stdcall IbLogiKeyboardSend(KeyboardInput * keys, uint32_t n);

static HANDLE pipe;

//Return 0 if succeeds.
extern "C" __declspec(dllexport) int __stdcall IbLogiInit() {
    pipe = CreateFileW(
        LR"(\\.\pipe\{B887DC25-1FF4-4409-95B9-A94EB4AAA3D6}.Keyboard)",
        GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );
    if (pipe == INVALID_HANDLE_VALUE) {
        //#TODO: start LGS?
        return GetLastError();
    }

    /*
    KeyboardInput keys[]{
        {'H', 1}, {'H', 0},
        {'E', 1}, {'E', 0},
        {'L', 1}, {'L', 0},
        {'L', 1}, {'L', 0},
        {'O', 1}, {'O', 0},
        {VK_LCONTROL, 1},
            {'A', 1}, {'A', 0},
        {VK_LCONTROL, 0}
    };
    IbLogiKeyboardSend(keys, std::size(keys));
    */

    return 0;
}

extern "C" __declspec(dllexport) void __stdcall IbLogiDestory() {
    CloseHandle(pipe);
}

//Return 0 if succeeds.
extern "C" __declspec(dllexport) int __stdcall IbLogiKeyboardSend(KeyboardInput* inputs, uint32_t n) {
    DWORD written;
    if (WriteFile(pipe, inputs, n * sizeof KeyboardInput, &written, nullptr))
        return 0;
    else
        return GetLastError();
}

/*
WINUSERAPI
UINT
WINAPI
SendInput(
    _In_ UINT cInputs,                     // number of input in the array
    _In_reads_(cInputs) LPINPUT pInputs,  // array of inputs
    _In_ int cbSize);                      // sizeof(INPUT)
*/

UINT WINAPI SendInputDetour(UINT cInputs, LPINPUT pInputs, int cbSize) {
    if (debug_) {
        std::wstringstream ss;
        ss << L"IbLogiLib.SendInput: ";
        for (UINT i = 0; i < cInputs; i++) {
            ss << (int)pInputs[i].ki.wVk << " " << (pInputs[i].ki.dwFlags & KEYEVENTF_KEYUP ? 0 : 1) << ", ";
        }
        OutputDebugStringW(ss.str().c_str());
    }

    static std::vector<KeyboardInput> inputs(6);
    for (UINT i = 0; i < cInputs; i++) {
        inputs.emplace_back((uint8_t)pInputs[i].ki.wVk, pInputs[i].ki.dwFlags & KEYEVENTF_KEYUP ?  0 : 1);
    }
    IbLogiKeyboardSend(inputs.data(), cInputs);
    inputs.clear();

    return cInputs;
}
static decltype(SendInputDetour) *SendInputTrue = SendInput;

extern "C" __declspec(dllexport) void __stdcall IbLogiSendInputBegin() {
    IbDetourAttach(&SendInputTrue, SendInputDetour);
}
extern "C" __declspec(dllexport) void __stdcall IbLogiSendInputEnd() {
    IbDetourDetach(&SendInputTrue, SendInputDetour);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    if (debug_)
        OutputDebugStringW((L"IbLogiLib.DllMain: " + std::to_wstring(ul_reason_for_call)).c_str());
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