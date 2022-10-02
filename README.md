# IbLogiSoftExt
Languages: [English](README.md), [简体中文](README.zh-Hans.md)

An extension for [Logitech Gaming Software](https://support.logi.com/hc/en-gb/articles/360025298053-Logitech-Gaming-Software).

## Features
- Remap G-keys to virtual-key codes (vkC0 + n), so that them can be detected by [AutoHotkey](https://www.autohotkey.com/), which enables you to:
  - Use the more professional AHK to perform hotkey actions instead of Lua.
  - Detect when your G-keys have been double-pressed, triple-pressed or long-pressed. (Remember to use "L" option when call KeyWait. [#2](../../issues/2))
  - Make your G-keys context-sensitive (perform different actions depending on specific conditions, such as different processes and windows). It's more configurable than LGS's Automatic Game Detection.
  - Define custom combinations of G-keys as hotkeys. (Just like G-Shift, but more powerful) ([#2](../../issues/2))
- Reduce the background memory usage of Logitech Gaming Software. It can reduce the memory from 70MB to 10MB, approximately.

## Supported version
- [Logitech Gaming Software v9.02.65 x64](https://github.com/Chaoses-Ib/IbLogiSoftExt/releases/download/v0.1/LGS.v9.02.65_x64.exe)

## Tested devices
Device | Description
------ | -----------
G600   | G6\~G20 work; G4\~G5 need some special settings, or use XButton1 and XButton2 instead
G300s  | G4\~G9 work
G700s  | G4\~G11 work. Not need Automatic Game Detection. ([#1](../../issues/1))

(If not mentioned, G1\~G3 need some special settings to work, but it's recommended to use LButton, RButton and MButton instead)

## Installation
1. Install [Logitech Gaming Software v9.02.65 x64](https://github.com/Chaoses-Ib/IbLogiSoftExt/releases/download/v0.1/LGS.v9.02.65_x64.exe), exit it via tray icon.
2. Download release files from [Releases](../../releases).
3. Put the `winmm.dll` in the release files into `C:\Program Files\Logitech Gaming Software` .
4. Restart Logitech Gaming Software, turn on Automatic Game Detection mode.
5. Run [RemappingTest.ahk](RemappingTest.ahk) (included in the release files) to test the remapping.

## Configuration
If you don't like the default behavior, you can modify it by following the steps below:
1. Create `winmm.dll.yaml` in the same directory the `winmm.dll` in.
2. Refer to the following content to edit it:
```yaml
# YAML
# LGS crashes when unplugging Logitech devices on some computers. This option will automatically restart LGS in this case. (IbParentProcessGuard.exe is required, and DisableWER.reg is recommanded to avoid Windows recording crashes.)
ProcessGuard: false
Memory:
  # If you have enough memory, you can turn it off.
  EmptyWorkingSetOnStartup: true
Mouse:
  # Whether or not to remap G1~G3. Sometimes may cause problems if turned on. (#1)
  RemapG123: false
```
(UTF-8 encoding)

## Credits
This project uses the following libraries:

- [Detours](https://github.com/microsoft/detours)
- [[Boost::ext].DI](https://github.com/boost-ext/di)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)
- [IbDllHijackLib](https://github.com/Chaoses-Ib/IbDllHijackLib)
- [IbWinCppLib](https://github.com/Chaoses-Ib/IbWinCppLib)

## See Also
- [IbInputSimulator](https://github.com/Chaoses-Ib/IbInputSimulator)