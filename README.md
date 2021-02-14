# IbLogitechGamingSoftwareExt
Languages: [English](README.md), [简体中文](README.zh-Hans.md)  
An extension for [Logitech Gaming Software](https://support.logi.com/hc/en-gb/articles/360025298053-Logitech-Gaming-Software).

## Features
* Remap G-keys to virtual-key codes (vkC0 + n), so that them can be detected by [AutoHotkey](https://www.autohotkey.com/), which enables you to:
  * Use the more professional AHK to perform hotkey actions instead of Lua.
  * Detect when your G-keys have been double-pressed, triple-pressed or long-pressed. (Remember to use "L" option when call KeyWait. [#2](../../issues/2))
  * Make your G-keys context-sensitive (perform different actions depending on specific conditions, such as different processes and windows). It's more configurable than LGS's Automatic Game Detection.
  * Define custom combinations of G-keys as hotkeys. (Just like G-Shift, but more powerful) ([#2](../../issues/2))
* Reduce the background memory usage of Logitech Gaming Software. It can reduce the memory from 70MB to 10MB, approximately.

## Supported Version
* Logitech Gaming Software 9.02.65 64-bit

## Tested Devices
Device | Description
------ | -----------
G600   | G6\~G20 work; G4\~G5 need some special settings, or use XButton1 and XButton2 instead
G300s  | G4\~G9 work
G700s  | G4\~G11 work. Not need Automatic Game Detection. ([#1](../../issues/1))

(If not mentioned, G1\~G3 need some special settings to work, but it's recommended to use LButton, RButton and MButton instead)

## Installation
1. Install [Logitech Gaming Software](https://support.logi.com/hc/en-gb/articles/360025298053-Logitech-Gaming-Software), exit it via tray icon.
1. Download release files from [Releases](../../releases).
1. Put the winmm.dll in the release files into C:\Program Files\Logitech Gaming Software .
1. Restart Logitech Gaming Software, turn on Automatic Game Detection mode.
1. Run [RemappingTest.ahk](RemappingTest.ahk)(included in the release files) to test the remapping.

## Configuration
If you don't like the default behavior, you can modify it by following the steps below:
1. Create winmm.dll.yaml in the same directory the winmm.dll in.
1. Refer to the following content to edit it:
```yaml
# YAML
Memory:
  # If you have enough memory, you can turn it off.
  EmptyWorkingSetOnStartup: true
Mouse:
  # Whether or not to remap G1~G3. Sometimes may cause problems if turned on. (#1)
  RemapG123: false
AHK:
  # Make AHK consider the sent VKs as physical keystrokes. Cause custom combination hotkeys not to work, but no more need to add "L" when call KeyWait. (#2)
  Physical_Ignore: false
```
(UTF-8 encoding)

## Credits
This project uses the following libraries:

* [Detours](https://github.com/microsoft/detours)
* [[Boost::ext].DI](https://github.com/boost-ext/di)
* [yaml-cpp](https://github.com/jbeder/yaml-cpp)
* [IbWinCppLib](https://github.com/Chaoses-Ib/IbWinCppLib)