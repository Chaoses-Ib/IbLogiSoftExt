#include "pch.h"
#include <thread>
#include <sstream>
#include <optional>
#include <Boost/di.hpp>
#include "helper.hpp"
#include <yaml-cpp/yaml.h>
#include "AHK.hpp"

namespace di = boost::di;

struct QString {
    static inline const wchar_t* (*utf16)(const QString* t);
};

class LogitechMouseExt {
    static inline struct {
        bool RemapG123;
        DWORD Physical_Ignore;  // = Physical_Ignore ? AHK::KEY_PHYS_IGNORE : 0
    } config;

    struct args
    {
        DWORD* pdword0;
        unsigned int* arg;
        const void* pqstring10;
        const void* family;
        const void* pqstring20;
    };
    static void LuaDispatchEventDetour(int64_t a1, int a2, int event, args* args) {
        const wchar* event_str = [event]{
            switch (event) {
            case 5: return L"G_PRESSED";
            case 6: return L"G_RELEASED";
            case 7: return L"M_PRESSED";
            case 8: return L"M_RELEASED";
            case 9: return L"@MODE_CHANGED"; //1~3, 4~6 for G-Shift
            case 10: return L"MOUSE_BUTTON_PRESSED";  //G6~G20, G1~G5 need setting
            case 11: return L"MOUSE_BUTTON_RELEASED";
            case 12: return L"GAME_CONTROLLER_BUTTON_PRESSED";
            case 13: return L"GAME_CONTROLLER_BUTTON_RELEASED";
            case 14: return L"GAME_CONTROLLER_BUTTON_PRESSED_AND_RELEASED";
            case 15: return L"AXIS_EVENT";
            case 16: return L"POV_EVENT";
            case 18: return L"@PROFILE_ACTIVATED";
            default: return L"";
            };
        }();
        //Crash under Qt 5.15.2
        //DebugOutput(wstringstream() << event_str << L"(" << event << L")" << L", " << *args->arg);

        //Remap G-keys
        if (event == 10 || event == 11) {
            [&] {
                uint32_t gkey = *args->arg;
                if (!config.RemapG123 && 1 <= gkey && gkey <= 3)
                    //do nothing when it's G123
                    return;

                INPUT input;
                input.type = INPUT_KEYBOARD;
                input.ki = {
                    WORD(0xC0 + gkey),
                    0,
                    DWORD(event == 11 ? KEYEVENTF_KEYUP : 0),
                    0,
                    (ULONG_PTR)GetMessageExtraInfo() | config.Physical_Ignore
                };
                SendInput(1, &input, sizeof INPUT);
            }();
        }

        LuaDispatchEvent(a1, a2, event, args); //even doesn't call can't stop
    }
    static inline decltype(LuaDispatchEventDetour)* LuaDispatchEvent;

    struct struct_a4
    {
        byte_t gap0[8];
        unsigned int* gbutton;
        const QString* field_10;
        const QString* pqstring18;
        QWORD qword20;
        const QString* pqstring28;
    };
    static void EventFunc13_GButtonDetour(int64_t a1, int a2, int a3, struct_a4* a4) {
        const wchar* event_str = [a3]{
            switch (a3) {
            case 10: return L"@CancelGbuttonFunc";
            case 11: return L"@SetGbuttonFunc";
            case 14: return L"@MouseButtonPressed"; //G6~G20, G1~G5 need setting
            case 15: return L"@MouseButtonReleased";
            case 29: return L"@ChangeCursorSetting";
            default: return L"";
            };
        }();
        auto tostring = [](const QString* q) {return q ? QString::utf16(q) : L"null"; };
        if (a3 == 14 || a3 == 15) {
            DebugOutput(wstringstream() << L"13: "
                << hex << a1 << L", " << oct
                << a2 << L", "
                << event_str << L"|" << a3 << L", "
                << *a4->gbutton << L", "
                << tostring(a4->field_10) << L", "
                << tostring(a4->pqstring18)
            );
        }
        else {
            DebugOutput(wstringstream() << L"13: "
                << hex << a1 << L", " << oct
                << a2 << L", "
                << event_str << L"|" << a3 << L", "
                << a4->gbutton << L", "
                << a4->field_10 << L", "
                << a4->pqstring18
            );
        }
        
        //Remap G-keys
        if (a3 == 14 || a3 == 15) {
            if (config.RemapG123)
                return;
            else
                //return when isn't G123
                if (!(1 <= *a4->gbutton && *a4->gbutton <= 3))
                    return;
        }

        EventFunc13_GButton(a1, a2, a3, a4);
    }
    static inline decltype(EventFunc13_GButtonDetour)* EventFunc13_GButton;
public:
    LogitechMouseExt() {
        //Get config
        config.RemapG123 = false;
        config.Physical_Ignore = 0;

        struct {
            bool EmptyWorkingSetOnStartup = true;
            bool ProcessGuard = false;
        } local_config;

        bool bad_file = false;
        YAML::Node yaml;
        try {
            yaml = YAML::LoadFile("winmm.dll.yaml");
        }
        catch (const YAML::BadFile&) {
            bad_file = true;
        }

        if (!bad_file) {
            if (auto node = yaml["Mouse"]["RemapG123"])
                config.RemapG123 = node.as<bool>();
            if (auto node = yaml["AHK"]["Physical_Ignore"])
                config.Physical_Ignore = node.as<bool>() ? AHK::KEY_PHYS_IGNORE : 0;

            if (auto node = yaml["Memory"]["EmptyWorkingSetOnStartup"])
                local_config.EmptyWorkingSetOnStartup = node.as<bool>();
            if (auto node = yaml["ProcessGuard"])
                local_config.ProcessGuard = node.as<bool>();
        }

        if (local_config.EmptyWorkingSetOnStartup) {
            /*
            static thread t([] {
                int count = 0;
                while (true) {
                    PROCESS_MEMORY_COUNTERS info;
                    GetProcessMemoryInfo((HANDLE)-1, &info, sizeof PROCESS_MEMORY_COUNTERS);
                    DebugOutput(wstringstream() << count << L" " << info.WorkingSetSize);
                    count += 50;
                    this_thread::sleep_for(50ms);
                };
                });
            t.detach();
            */

            thread t([] {
                this_thread::sleep_for(30s);
                EmptyWorkingSet(GetCurrentProcess());
            });
            t.detach();
        }

        //Attach
        Module LCore = *ModuleFactory::CurrentProcess();
        LuaDispatchEvent = LCore.base.offset(0x71CC40);
        //DebugOutput(wstringstream() << LuaDispatchEvent);
        auto status = IbDetourAttach(&LuaDispatchEvent, LuaDispatchEventDetour);
        //DebugOutput(wstringstream() << LuaDispatchEvent << L" " << status);

        EventFunc13_GButton = LCore.base.offset(0x80D410);
        IbDetourAttach(&EventFunc13_GButton, EventFunc13_GButtonDetour);

        Module Qt5Core = *ModuleFactory::Find(L"Qt5Core.dll");
        QString::utf16 = (decltype(QString::utf16))GetProcAddress(Qt5Core.handle, "?utf16@QString@@QEBAPEBGXZ");
        DebugOutput(wstringstream() << L"QString::utf16:" << QString::utf16);

        if (local_config.ProcessGuard) {
            STARTUPINFO si{ sizeof(STARTUPINFO) };
            PROCESS_INFORMATION pi;
            wchar cmdline[] = L"/minimized";
            CreateProcessW(L"IbParentProcessGuard.exe", cmdline, nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
    ~LogitechMouseExt() {
        //IbDetourDetach(&LuaDispatchEvent, LuaDispatchEventDetour);
    }
};

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    static optional<LogitechMouseExt> ext;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DebugOutput(L"Load");
        ext = di::make_injector().create<LogitechMouseExt>();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        ext.reset();
        break;
    }
    return TRUE;
}