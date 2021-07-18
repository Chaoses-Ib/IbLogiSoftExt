#include "pch.h"
#include "driver.hpp"
#include <IbWinCppLib/WinCppLib.hpp>
#include "../Injector/helper.hpp"
#include <sstream>

using namespace ib;

extern HANDLE device = 0;
extern bool has_acceleration = false;

void DriverSend(INPUT inputs[], uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        DWORD type = inputs[i].type;

        uint32_t j = i + 1;
        while (j < n && inputs[j].type == type)
            j++;

        switch (type) {
        case INPUT_KEYBOARD:
            DriverKeyboardSend(inputs + i, j - i);
            break;
        case INPUT_MOUSE:
            DriverMouseSend(inputs + i, j - i);
            break;
        }
    }
}

struct Button {
    bool LButton : 1;
    bool RButton : 1;
    bool MButton : 1;
    bool XButton1 : 1;
    bool XButton2 : 1;
    bool unknown : 3;
};

struct MouseReport {
    union {
        Button button;
        Byte button_byte;
    };
    int8_t x;
    int8_t y;
    Byte unknown_W;  //#TODO: Wheel?
    Byte unknown_T;  //#TODO: T?
private:
    void assert_size() {
        static_assert(sizeof MouseReport == 5);
    }
};

//#TODO
LONG CompensateWinAcceleration(LONG x) {
    static struct {
        int threshold[2];
        int acceleration;
    } mouse;
    static int speed = -1;  //1~20. #TODO: 0 when disabled?
    if (speed == -1) {  //#TODO: may change
        SystemParametersInfoW(SPI_GETMOUSE, 0, &mouse, 0);
        SystemParametersInfoW(SPI_GETMOUSESPEED, 0, &speed, 0);
        DebugOutput(wstringstream() << L"LogiLib.AvoidMouseAcceleration: "
            << mouse.acceleration << L", " << mouse.threshold[0] << L"," << mouse.threshold[1]
            << L", " << speed);
        //1, 6,10, 10
    }

    /*
    if (mouse.acceleration) {
        if (x > mouse.threshold[0])
            x *= 2;
        if (x > mouse.threshold[1] && mouse.acceleration == 2)
            x *= 2;
    }
    return x * speed / 10;
    */

    return x;
}

int8_t CompensateLgsAcceleration(int8_t x) {
    int8_t abs_x = abs(x);
    int8_t sign = x > 0 ? 1 : -1;

    if (abs_x <= 5)
        return x;
    else if (abs_x <= 10)
        return sign * (abs_x + 1);
    else
        return sign * (int8_t)round(0.6156218196 * abs_x + 4.45777444629);
}

void MouseSend(MouseReport report) {
    const DWORD IOCTL_BUSENUM_PLAY_MOUSEMOVE = 0x2A2010;
    DWORD bytes_returned;

    if (has_acceleration && (report.x || report.y)) {
        static MouseReport report11{ {0}, 1, 1, 0, 0 };  //{ 0, 1, 1, 0, 0} will get { {6}, 0, 0, 0, 0}
        //DebugOutput(wstringstream() << L"LogiLib.Mouse.Report11: " << report11.button_byte << L", " << report11.x << L", " << report11.y);
        DeviceIoControl(device, IOCTL_BUSENUM_PLAY_MOUSEMOVE, (void*)&report11, sizeof MouseReport, nullptr, 0, &bytes_returned, nullptr);
        report11.x = -report11.x;
        report11.y = -report11.y;

        report.x = CompensateLgsAcceleration(report.x);
        report.y = CompensateLgsAcceleration(report.y);
    }

    DeviceIoControl(device, IOCTL_BUSENUM_PLAY_MOUSEMOVE, &report, sizeof MouseReport, nullptr, 0, &bytes_returned, nullptr);
}

void DriverMouseSend(INPUT inputs[], uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        MouseReport report{};  //clear every time

        MOUSEINPUT& mi = inputs[i].mi;
        DebugOutput(wstringstream() << L"LogiLib.MouseInput: " << mi.dwFlags << ", " << mi.dx << ", " << mi.dy);

        //#TODO: move and then click, or click and then move? former?

        //#TODO: MOUSEEVENTF_MOVE_NOCOALESCE, MOUSEEVENTF_VIRTUALDESK
        if (mi.dwFlags & MOUSEEVENTF_MOVE || mi.dwFlags & MOUSEEVENTF_MOVE_NOCOALESCE) {
            if (mi.dwFlags & MOUSEEVENTF_ABSOLUTE) {
                static POINT screen;  //#TODO: may change
                if (!screen.x) {
                    screen.x = GetSystemMetrics(SM_CXSCREEN);
                    screen.y = GetSystemMetrics(SM_CYSCREEN);
                }
                // mi.dx = round(x / screen.x * 65536)
                mi.dx = mi.dx * screen.x / 65536;
                mi.dy = mi.dy * screen.y / 65536;

                POINT point;
                GetCursorPos(&point);
                DebugOutput(wstringstream() << L"LogiLib.MouseAbsolute: " << mi.dx << ", " << mi.dy << ", from " << point.x << ", " << point.y);
                mi.dx -= point.x;
                mi.dy -= point.y;
            }

            while (abs(mi.dx) > 127 || abs(mi.dy) > 127) {
                if (abs(mi.dx) > 127) {
                    report.x = mi.dx > 0 ? 127 : -127;
                    mi.dx -= report.x;
                }
                else {
                    report.x = 0;
                }

                if (abs(mi.dy) > 127) {
                    report.y = mi.dy > 0 ? 127 : -127;
                    mi.dy -= report.y;
                }
                else {
                    report.y = 0;
                }

                DebugOutput(wstringstream() << L"LogiLib.Mouse: " << report.button_byte << L", "
                    << report.x << L", " << report.y << L" (" << CompensateLgsAcceleration(report.x) << L", " << CompensateLgsAcceleration(report.y) << L")"
                    << L", " << mi.dx << L", " << mi.dy);
                MouseSend(report);
            }

            report.x = (uint8_t)mi.dx;
            report.y = (uint8_t)mi.dy;
        }

#define CODE_GENERATE(down, up, member)  \
        if (mi.dwFlags & down || mi.dwFlags & up)  \
            report.button.##member = mi.dwFlags & down;
        CODE_GENERATE(MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP, LButton)  //#TODO: may be switched?
            CODE_GENERATE(MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP, RButton)
            CODE_GENERATE(MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP, MButton)
#undef CODE_GENERATE
            if (mi.dwFlags & MOUSEEVENTF_XDOWN || mi.dwFlags & MOUSEEVENTF_XUP) {
                bool down = mi.dwFlags & MOUSEEVENTF_XDOWN;
                switch (mi.mouseData) {
                case XBUTTON1: report.button.XButton1 = down; break;
                case XBUTTON2: report.button.XButton2 = down; break;
                }
            }

        DebugOutput(wstringstream() << L"LogiLib.Mouse: " << report.button_byte << L", "
            << report.x << L", " << report.y << L" (" << CompensateLgsAcceleration(report.x) << L", " << CompensateLgsAcceleration(report.y) << L")");
        MouseSend(report);
    }
}


struct Modifier {
    bool LCtrl : 1;
    bool LShift : 1;
    bool LAlt : 1;
    bool LGui : 1;
    bool RCtrl : 1;
    bool RShift : 1;
    bool RAlt : 1;
    bool RGui : 1;
};

struct KeyboardReport {
    union {
        Modifier modifier;
        Byte modifier_byte;
    };
    Byte reserved;
    Byte keys[6];
private:
    void assert_size() {
        static_assert(sizeof KeyboardReport == 8);
    }
};

uint8_t VkToUsage(uint8_t vkCode);

static KeyboardReport report;

SHORT DriverGetKeyState(int vKey, decltype(GetAsyncKeyState) f) {
    switch (vKey) {
#define CODE_GENERATE(vk, member)  case vk: return report.modifier.##member << 15;
        CODE_GENERATE(VK_LCONTROL, LCtrl)
            CODE_GENERATE(VK_RCONTROL, RCtrl)
            CODE_GENERATE(VK_LSHIFT, LShift)
            CODE_GENERATE(VK_RSHIFT, RShift)
            CODE_GENERATE(VK_LMENU, LAlt)
            CODE_GENERATE(VK_RMENU, RAlt)
            CODE_GENERATE(VK_LWIN, LGui)
            CODE_GENERATE(VK_RWIN, RGui)
#undef CODE_GENERATE
    default:
        return f(vKey);
    }
}

void DriverSyncKeyStates() {
    //#TODO: GetKeyboardState() ?
    //static bool states[256];  //down := true
#define CODE_GENERATE(vk, member)  report.modifier.##member = GetAsyncKeyState(vk) & 0x8000;
    CODE_GENERATE(VK_LCONTROL, LCtrl)
        CODE_GENERATE(VK_RCONTROL, RCtrl)
        CODE_GENERATE(VK_LSHIFT, LShift)
        CODE_GENERATE(VK_RSHIFT, RShift)
        CODE_GENERATE(VK_LMENU, LAlt)
        CODE_GENERATE(VK_RMENU, RAlt)
        CODE_GENERATE(VK_LWIN, LGui)
        CODE_GENERATE(VK_RWIN, RGui)
#undef CODE_GENERATE
}

void DriverKeyboardSend(INPUT inputs[], uint32_t n) {
    DWORD bytes_returned;
    for (uint32_t i = 0; i < n; i++) {
        bool keydown = !(inputs[i].ki.dwFlags & KEYEVENTF_KEYUP);
        switch (inputs[i].ki.wVk) {

#define CODE_GENERATE(vk, member)  \
        case vk:  \
            report.modifier.##member = keydown;  \
            break;
            CODE_GENERATE(VK_LCONTROL, LCtrl)
                CODE_GENERATE(VK_RCONTROL, RCtrl)
                CODE_GENERATE(VK_LSHIFT, LShift)
                CODE_GENERATE(VK_RSHIFT, RShift)
                CODE_GENERATE(VK_LMENU, LAlt)
                CODE_GENERATE(VK_RMENU, RAlt)
                CODE_GENERATE(VK_LWIN, LGui)
                CODE_GENERATE(VK_RWIN, RGui)
#undef CODE_GENERATE

        default:
            uint8_t usage = VkToUsage((uint8_t)inputs[i].ki.wVk);;
            if (keydown) {
                for (int i = 0; i < 6; i++) {
                    if (report.keys[i] == 0) {
                        report.keys[i] = usage;
                        break;
                    }
                }
                //full
            }
            else {
                for (int i = 0; i < 6; i++) {
                    if (report.keys[i] == usage) {
                        report.keys[i] = 0;
                        //#TODO: move to left?
                        break;
                    }
                }
            }
        }

        DebugOutput(wstringstream() << L"LogiLib.Kerboard: " << report.modifier_byte << L", " << report.keys[0] << ", " << report.keys[1]
            << " (" << inputs[i].ki.wVk << ", " << inputs[i].ki.dwFlags << ")");
        DeviceIoControl(device, 0x2A200C, &report, sizeof KeyboardReport, nullptr, 0, &bytes_returned, nullptr);
    }
}

//#TODO: fully test
uint8_t VkToUsage(uint8_t vkCode) {
    switch (vkCode) {
    case 0x00: return 0x0000;
    case 0x01: return 0x0000;  //VK_LBUTTON
    case 0x02: return 0x0000;  //VK_RBUTTON
    case 0x03: return 0x9B;  //VK_CANCEL
    case 0x04: return 0x0000;  //VK_MBUTTON
    case 0x05: return 0x0000;  //VK_XBUTTON1
    case 0x06: return 0x0000;  //VK_XBUTTON2
    case 0x07: return 0x00;  //reserved
    case 0x08: return 0x2A;  //VK_BACK
    case 0x09: return 0x2B;  //VK_TAB
    case 0x0A: return 0x00;  //reserved
    case 0x0B: return 0x00;  //reserved
    case 0x0C: return 0x0000;  //VK_CLEAR
    case 0x0D: return 0x28;  //VK_RETURN
    case 0x0E: return 0x00;  //reserved
    case 0x0F: return 0x00;  //reserved
    case 0x10: return 0x0000;  //VK_SHIFT
    case 0x11: return 0x0000;  //VK_CONTROL
    case 0x12: return 0x0000;  //VK_MENU
    case 0x13: return 0x48;  //VK_PAUSE
    case 0x14: return 0x39;  //VK_CAPITAL
    case 0x15: return 0x0000;  //VK_KANA|VK_HANGEUL|VK_HANGUL
    case 0x16: return 0x00;  //unassigned
    case 0x17: return 0x0000;  //VK_JUNJA
    case 0x18: return 0x0000;  //VK_FINAL
    case 0x19: return 0x0000;  //VK_HANJA|VK_KANJI
    case 0x1A: return 0x00;  //unassigned
    case 0x1B: return 0x29;  //VK_ESCAPE
    case 0x1C: return 0x0000;  //VK_CONVERT
    case 0x1D: return 0x0000;  //VK_NONCONVERT
    case 0x1E: return 0x0000;  //VK_ACCEPT
    case 0x1F: return 0x0000;  //VK_MODECHANGE
    case 0x20: return 0x2C;  //VK_SPACE
    case 0x21: return 0x4B;  //VK_PRIOR
    case 0x22: return 0x4E;  //VK_NEXT
    case 0x23: return 0x4D;  //VK_END
    case 0x24: return 0x4A;  //VK_HOME
    case 0x25: return 0x50;  //VK_LEFT
    case 0x26: return 0x52;  //VK_UP
    case 0x27: return 0x4F;  //VK_RIGHT
    case 0x28: return 0x51;  //VK_DOWN
    case 0x29: return 0x0000;  //VK_SELECT
    case 0x2A: return 0x0000;  //VK_PRINT
    case 0x2B: return 0x0000;  //VK_EXECUTE
    case 0x2C: return 0x46;  //VK_SNAPSHOT
    case 0x2D: return 0x49;  //VK_INSERT
    case 0x2E: return 0x4C;  //VK_DELETE
    case 0x2F: return 0x0000;  //VK_HELP
    case 0x3A:
    case 0x3B:
    case 0x3C:
    case 0x3D:
    case 0x3E:
    case 0x3F:
    case 0x40: return 0x00;  //unassigned
    case 0x5B: return 0xE3;  //VK_LWIN
    case 0x5C: return 0xE7;  //VK_RWIN
    case 0x5D: return 0x65;  //VK_APPS
    case 0x5E: return 0x00;  //reserved
    case 0x5F: return 0x0000;  //VK_SLEEP

    case 0x6A: return 0x55;  //VK_MULTIPLY
    case 0x6B: return 0x57;  //VK_ADD
    case 0x6C: return 0x0000;  //VK_SEPARATOR
    case 0x6D: return 0x56;  //VK_SUBTRACT
    case 0x6E: return 0x63;  //VK_DECIMAL
    case 0x6F: return 0x54;  //VK_DIVIDE

    case 0x88: return 0x0000;  //VK_NAVIGATION_VIEW
    case 0x89: return 0x0000;  //VK_NAVIGATION_MENU
    case 0x8A: return 0x0000;  //VK_NAVIGATION_UP
    case 0x8B: return 0x0000;  //VK_NAVIGATION_DOWN
    case 0x8C: return 0x0000;  //VK_NAVIGATION_LEFT
    case 0x8D: return 0x0000;  //VK_NAVIGATION_RIGHT
    case 0x8E: return 0x0000;  //VK_NAVIGATION_ACCEPT
    case 0x8F: return 0x0000;  //VK_NAVIGATION_CANCEL
    case 0x90: return 0x53;  //VK_NUMLOCK
    case 0x91: return 0x47;  //VK_SCROLL
    case 0x92: return 0x0000;  //VK_OEM_NEC_EQUAL|VK_OEM_FJ_JISHO
    case 0x93: return 0x0000;  //VK_OEM_FJ_MASSHOU
    case 0x94: return 0x0000;  //VK_OEM_FJ_TOUROKU
    case 0x95: return 0x0000;  //VK_OEM_FJ_LOYA
    case 0x96: return 0x0000;  //VK_OEM_FJ_ROYA
    case 0x97:
    case 0x98:
    case 0x99:
    case 0x9A:
    case 0x9B:
    case 0x9C:
    case 0x9D:
    case 0x9E:
    case 0x9F: return 0x00;  //unassigned
    case 0xA0: return 0xE1;  //VK_LSHIFT
    case 0xA1: return 0xE5;  //VK_RSHIFT
    case 0xA2: return 0xE0;  //VK_LCONTROL
    case 0xA3: return 0xE4;  //VK_RCONTROL
    case 0xA4: return 0xE2;  //VK_LMENU
    case 0xA5: return 0xE6;  //VK_RMENU
    case 0xA6: return 0x0000;  //VK_BROWSER_BACK
    case 0xA7: return 0x0000;  //VK_BROWSER_FORWARD
    case 0xA8: return 0x0000;  //VK_BROWSER_REFRESH
    case 0xA9: return 0x0000;  //VK_BROWSER_STOP
    case 0xAA: return 0x0000;  //VK_BROWSER_SEARCH
    case 0xAB: return 0x0000;  //VK_BROWSER_FAVORITES
    case 0xAC: return 0x0000;  //VK_BROWSER_HOME
    case 0xAD: return 0x0000;  //VK_VOLUME_MUTE
    case 0xAE: return 0x0000;  //VK_VOLUME_DOWN
    case 0xAF: return 0x0000;  //VK_VOLUME_UP
    case 0xB0: return 0x0000;  //VK_MEDIA_NEXT_TRACK
    case 0xB1: return 0x0000;  //VK_MEDIA_PREV_TRACK
    case 0xB2: return 0x0000;  //VK_MEDIA_STOP
    case 0xB3: return 0x0000;  //VK_MEDIA_PLAY_PAUSE
    case 0xB4: return 0x0000;  //VK_LAUNCH_MAIL
    case 0xB5: return 0x0000;  //VK_LAUNCH_MEDIA_SELECT
    case 0xB6: return 0x0000;  //VK_LAUNCH_APP1
    case 0xB7: return 0x0000;  //VK_LAUNCH_APP2
    case 0xB8:
    case 0xB9: return 0x00;  //reserved
    case 0xBA: return 0x33;  //VK_OEM_1   // ';:' for US
    case 0xBB: return 0x2E;  //VK_OEM_PLUS   // '+' any country
    case 0xBC: return 0x36;  //VK_OEM_COMMA   // ',' any country
    case 0xBD: return 0x2D;  //VK_OEM_MINUS   // '-' any country
    case 0xBE: return 0x37;  //VK_OEM_PERIOD   // '.' any country
    case 0xBF: return 0x38;  //VK_OEM_2   // '/?' for US
    case 0xC0: return 0x35;  //VK_OEM_3   // '`~' for US
    case 0xC1: return 0x00;  //reserved
    case 0xC2: return 0x00;  //reserved
    case 0xC3: return 0x00;  //VK_GAMEPAD_A // reserved
    case 0xC4: return 0x00;  //VK_GAMEPAD_B // reserved
    case 0xC5: return 0x00;  //VK_GAMEPAD_X // reserved
    case 0xC6: return 0x00;  //VK_GAMEPAD_Y // reserved
    case 0xC7: return 0x00;  //VK_GAMEPAD_RIGHT_SHOULDER // reserved
    case 0xC8: return 0x00;  //VK_GAMEPAD_LEFT_SHOULDER // reserved
    case 0xC9: return 0x00;  //VK_GAMEPAD_LEFT_TRIGGER // reserved
    case 0xCA: return 0x00;  //VK_GAMEPAD_RIGHT_TRIGGER // reserved
    case 0xCB: return 0x00;  //VK_GAMEPAD_DPAD_UP // reserved
    case 0xCC: return 0x00;  //VK_GAMEPAD_DPAD_DOWN // reserved
    case 0xCD: return 0x00;  //VK_GAMEPAD_DPAD_LEFT // reserved
    case 0xCE: return 0x00;  //VK_GAMEPAD_DPAD_RIGHT // reserved
    case 0xCF: return 0x00;  //VK_GAMEPAD_MENU // reserved
    case 0xD0: return 0x00;  //VK_GAMEPAD_VIEW // reserved
    case 0xD1: return 0x00;  //VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON // reserved
    case 0xD2: return 0x00;  //VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON // reserved
    case 0xD3: return 0x00;  //VK_GAMEPAD_LEFT_THUMBSTICK_UP // reserved
    case 0xD4: return 0x00;  //VK_GAMEPAD_LEFT_THUMBSTICK_DOWN // reserved
    case 0xD5: return 0x00;  //VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT // reserved
    case 0xD6: return 0x00;  //VK_GAMEPAD_LEFT_THUMBSTICK_LEFT // reserved
    case 0xD7: return 0x00;  //VK_GAMEPAD_RIGHT_THUMBSTICK_UP // reserved
    case 0xD8: return 0x00;  //VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN // reserved
    case 0xD9: return 0x00;  //VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT // reserved
    case 0xDA: return 0x00;  //VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT // reserved
    case 0xDB: return 0x2F;  //VK_OEM_4  //  '[{' for US
    case 0xDC: return 0x31;  //VK_OEM_5  //  '\|' for US
    case 0xDD: return 0x30;  //VK_OEM_6  //  ']}' for US
    case 0xDE: return 0x34;  //VK_OEM_7  //  ''"' for US
    case 0xDF: return 0x0000;  //VK_OEM_8
    case 0xE0: return 0x0000;  //reserved
    case 0xE1: return 0x0000;  //VK_OEM_AX  //  'AX' key on Japanese AX kbd
    case 0xE2: return 0x64;  //VK_OEM_102  //  "<>" or "\|" on RT 102-key kbd.
    case 0xE3: return 0x0000;  //VK_ICO_HELP  //  Help key on ICO
    case 0xE4: return 0x0000;  //VK_ICO_00  //  00 key on ICO
    case 0xE5: return 0x0000;  //VK_PROCESSKEY
    case 0xE6: return 0x0000;  //VK_ICO_CLEAR
    case 0xE7: return 0x0000;  //VK_PACKET
    case 0xE8: return 0x0000;  //unassigned
    case 0xE9: return 0x0000;  //VK_OEM_RESET
    case 0xEA: return 0x0000;  //VK_OEM_JUMP
    case 0xEB: return 0x0000;  //VK_OEM_PA1
    case 0xEC: return 0x0000;  //VK_OEM_PA2
    case 0xED: return 0x0000;  //VK_OEM_PA3
    case 0xEE: return 0x0000;  //VK_OEM_WSCTRL
    case 0xEF: return 0x0000;  //VK_OEM_CUSEL
    case 0xF0: return 0x0000;  //VK_OEM_ATTN
    case 0xF1: return 0x0000;  //VK_OEM_FINISH
    case 0xF2: return 0x0000;  //VK_OEM_COPY
    case 0xF3: return 0x0000;  //VK_OEM_AUTO
    case 0xF4: return 0x0000;  //VK_OEM_ENLW
    case 0xF5: return 0x0000;  //VK_OEM_BACKTAB
    case 0xF6: return 0x0000;  //VK_ATTN
    case 0xF7: return 0x0000;  //VK_CRSEL
    case 0xF8: return 0x0000;  //VK_EXSEL
    case 0xF9: return 0x0000;  //VK_EREOF
    case 0xFA: return 0x0000;  //VK_PLAY
    case 0xFB: return 0x0000;  //VK_ZOOM
    case 0xFC: return 0x0000;  //VK_NONAME
    case 0xFD: return 0x0000;  //VK_PA1
    case 0xFE: return 0x0000;  //VK_OEM_CLEAR
    case 0xFF: return 0x0000;
    default:
        if ('A' <= vkCode and vkCode <= 'Z')
            return 0x04 + vkCode - 'A';
        else if ('0' <= vkCode and vkCode <= '9')
            return vkCode == '0' ? 0x27 : 0x1E + vkCode - '1';
        else if (VK_NUMPAD0 <= vkCode and vkCode <= VK_NUMPAD9)
            return vkCode == VK_NUMPAD0 ? 0x62 : 0x59 + vkCode - VK_NUMPAD1;
        else if (VK_F1 <= vkCode and vkCode <= VK_F24)
            return vkCode <= VK_F12
            ? 0x3A + vkCode - VK_F1
            : 0x68 + vkCode - VK_F13;
    }
}