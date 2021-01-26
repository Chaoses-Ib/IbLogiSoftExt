# IbLogitechGamingSoftwareExt
Languages: [English](README.md), [简体中文](README.zh-Hans.md)  
An extension for [Logitech Gaming Software](https://support.logi.com/hc/en-gb/articles/360025298053-Logitech-Gaming-Software).

## Features
* Remap G-keys to virtual-key codes (vkC0 + n), so that them can be received by [AutoHotkey](https://www.autohotkey.com/).

## Supported Version
* Logitech Gaming Software 9.02.65 64-bit

## Installation
1. Install [Logitech Gaming Software](https://support.logi.com/hc/en-gb/articles/360025298053-Logitech-Gaming-Software), exit it (via tray icon).
1. Put winmm.dll into C:\Program Files\Logitech Gaming Software .
1. Restart Logitech Gaming Software, turn on Automatic Game Detection mode.
1. Run RemappingTest.ahk to test the remapping.

## Credits
This project uses the following libraries:

* [Detours](https://github.com/microsoft/detours)
* [[Boost::ext].DI](https://github.com/boost-ext/di)
