# IbLogitechGamingSoftwareExt
语言: [English](README.md), [简体中文](README.zh-Hans.md)  
一个 [Logitech 游戏软件](https://support.logi.com/hc/zh-cn/articles/360025298053) 的扩展。

## 功能
* 将G键重映射为虚拟键码（vkC0 + n），从而使 [AutoHotkey](https://www.autohotkey.com/) 可以检测到G键。这样你就可以：
  * 使用比Lua更专业的AHK来执行热键动作。
  * 检测G键的双击、三击和长按。（记得调用 KeyWait 时使用“L”选项。[#2](../../issues/2)）
  * 让G键上下文敏感（根据不同条件执行不同动作，比如不同的进程和窗口）。比LGS的自动游戏检测具有更强的可配置性。
  * 定义G键的自定义组合热键。（就像G切换，但更强大）（[#2](../../issues/2)）
* 减少 Logitech 游戏软件的后台内存使用量。可以大约把内存从 70MB 减少到 10MB。

## 支持版本
* Logitech 游戏软件 9.02.65 64位

## 已测试设备
设备   | 说明
------ | -----------
G600   | G6\~G20 正常；G4\~G5 需要一些特殊设置，或者使用 XButton1 和 XButton2 代替
G300s  | G4\~G9 正常
G700s  | G4\~G11 正常。不需要自动游戏检测。（[#1](../../issues/1)）

（如果没有说明的话，G1\~G3 需要一些特殊设置才能工作，不过推荐使用 LButton、RButton 和 MButton 代替）

## 安装
1. 安装 [Logitech 游戏软件](https://support.logi.com/hc/zh-cn/articles/360025298053)，通过托盘图标退出。
1. 从 [Releases](../../releases) 下载发行文件。
1. 把发行文件中的 winmm.dll 放进 C:\Program Files\Logitech Gaming Software 。
1. 重新启动 Logitech 游戏软件，开启自动游戏检测模式。
1. 运行 [RemappingTest.ahk](RemappingTest.ahk)（包含在发行文件中）来测试重映射。

## 配置
如果你不喜欢默认行为，你可以通过以下步骤来修改它：
1. 在 winmm.dll 的相同目录下创建 winmm.dll.yaml。
1. 参考以下内容进行编辑：
```yaml
# YAML
ProcessGuard: false
Memory:
  # 如果你内存够用，可以关掉。
  EmptyWorkingSetOnStartup: true
Mouse:
  # 是否重映射G1~G3。如果开启有时可能会出现问题。（#1）
  RemapG123: false
AHK:
  # 让AHK将发送的VK当作物理按键。会导致自定义组合热键失效，但调用 KeyWait 时也不用再加“L”了。（#2）
  Physical_Ignore: false
```
（UTF-8 编码）

## 鸣谢
本项目使用了以下库：

* [Detours](https://github.com/microsoft/detours)
* [[Boost::ext].DI](https://github.com/boost-ext/di)
* [yaml-cpp](https://github.com/jbeder/yaml-cpp)
* [IbWinCppLib](https://github.com/Chaoses-Ib/IbWinCppLib)