# IbLogitechGamingSoftwareExt
语言: [English](README.md), [简体中文](README.zh-Hans.md)  
一个 [Logitech 游戏软件](https://support.logi.com/hc/zh-cn/articles/360025298053) 的扩展。

## 功能
* 将G键重映射为虚拟键码（vkC0 + n），从而使 [AutoHotkey](https://www.autohotkey.com/) 可以接收到G键。

## 支持版本
* Logitech 游戏软件 9.02.65 64位

## 安装
1. 安装 [Logitech 游戏软件](https://support.logi.com/hc/zh-cn/articles/360025298053)，退出（通过托盘图标）。
1. 把 winmm.dll 放进 C:\Program Files\Logitech Gaming Software 。
1. 重新启动 Logitech 游戏软件，开启自动游戏探测模式。
1. 运行 RemappingTest.ahk 来测试重映射。

## 鸣谢
本项目使用了以下库：

* [Detours](https://github.com/microsoft/detours)
* [[Boost::ext].DI](https://github.com/boost-ext/di)
