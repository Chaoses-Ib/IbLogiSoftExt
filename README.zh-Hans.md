# IbLogitechGamingSoftwareExt
语言: [English](README.md), [简体中文](README.zh-Hans.md)  
一个 [Logitech 游戏软件](https://support.logi.com/hc/zh-cn/articles/360025298053) 的扩展。

## 功能
* 将G键重映射为虚拟键码（vkC0 + n），从而使 [AutoHotkey](https://www.autohotkey.com/) 可以接收到G键。

## 支持版本
* Logitech 游戏软件 9.02.65 64位

## 已测试设备
设备   | 说明
------ | -----------
G600   | G6\~G20 正常；G1\~G5 需要一些特殊设置，或者使用 XButton1 和 XButton2 代替
G300s  | G4\~G9 正常

（如果没有说明的话，G1\~G3 需要一些特殊设置才能工作，不过推荐使用 LButton、RButton 和 MButton 代替）

## 安装
1. 安装 [Logitech 游戏软件](https://support.logi.com/hc/zh-cn/articles/360025298053)，通过托盘图标退出。
1. 从 [Releases](../../releases) 下载发行文件。
1. 把发行文件中的 winmm.dll 放进 C:\Program Files\Logitech Gaming Software 。
1. 重新启动 Logitech 游戏软件，开启自动游戏检测模式。
1. 运行 [RemappingTest.ahk](RemappingTest.ahk)（包含在发行文件中）来测试重映射。

## 鸣谢
本项目使用了以下库：

* [Detours](https://github.com/microsoft/detours)
* [[Boost::ext].DI](https://github.com/boost-ext/di)
