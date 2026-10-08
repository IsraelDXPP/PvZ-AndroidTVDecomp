# LawnProjectTV

把《植物大战僵尸》电视 / Android 版里的 `libGameMain.so` 还原成可在 Windows 上编译、运行的 C++ 工程。游戏逻辑对照原二进制还原，桌面平台层使用 SDL3 窗口、输入和音频，以及 bgfx 渲染。

当前状态是**重建进行中**。工程可以编出 `LawnTV.exe` 并进入帧循环，但大量函数仍是对着反编译结果逐段誊写的，启动期的堆损坏和布局偏差还在查。不要把它当成能完整通关的客户端。

原作版权归 PopCap / EA。本仓库只放重建出来的代码和构建说明，不包含游戏资源。

## 目录

| 路径 | 内容 |
| --- | --- |
| `Lawn` | 游戏逻辑：`LawnApp`、`Board`、植物、僵尸、关卡、对话框 |
| `Sexy` | PopCap SexyApp 框架的还原（控件、资源、渲染接口） |
| `Sexy/src/Platform` | SDL3 窗口、输入、手柄、音频、文件及 bgfx 渲染 |
| `Sexy/include/Runtime` | ARM 反编译结果的运行库兼容实现和 `Sexy::String` |
| `Lawn/src/main.cpp` | SDL3 入口和帧循环 |
| `vcpkg.json` | 第三方库清单 |
| `Tools` | [SCBP 着色器编译打包脚本与格式说明](Tools/scbp.md) |
| `Shaders` | [游戏着色器源码及多平台 SCBP 包](Shaders/README.md) |

原 Android 构建从 JNI 进 `Native::BridgeApp`，由 `Sexy::AndroidAppDriver` 驱动。当前由 SDL3 的 `main` 构造 `LawnApp` 并跑帧循环。

资源路径按 APK 里的 `files/` 来解析。把资源解到可执行文件旁边的 `assets/`（或 `assets/files/`）即可；程序会在发现 `files` 子目录时自动走进去。也可以用 `--assets` 指到别的目录。

## 构建

需要：

- Visual Studio 2022 或更新（工程工具集是 `v145`，即 VS 2026；用 2022 时把工具集改成已安装的版本）
- 带 manifest 模式的 [vcpkg](https://github.com/microsoft/vcpkg)，并集成进 MSBuild
- **Win32（x86）**。类布局按 32 位 ARM 二进制的字段偏移还原，x64 配置虽然写在工程里，但对象大小对不上。

依赖见 `vcpkg.json`：SDL3、SDL3_net、bgfx（multithreaded、tools）、SDL3_image（PNG、JPEG）、GLM、zlib、minizip-ng、bzip2、libogg、libvorbis、libopenmpt、nlohmann-json、pugixml、stb。SDL3_image 优先解码图像，stb 补充其他格式；Vorbis/openmpt 解码后交给 SDL3 音频流，SDL3_net 负责日志接口的主机名解析。游戏字体使用原资源的位图字体，无需 FreeType。

```text
vcpkg install --triplet x86-windows
```

用 Visual Studio 打开 `Lawn.slnx`，选 **Debug | x86** 或 **Release | x86**（项目平台为 Win32）生成。产物是仓库根下的：

```text
build\Win32\<Configuration>\LawnTV.exe
```

vcpkg 的 MSBuild 集成自动复制所需 DLL，Lawn 项目自动把 `Shaders/*.scbp` 复制到 exe 旁边的 `assets/shaders`。另外放上游戏资源目录；使用 `--assets` 时把着色器也复制到对应目录的 `shaders` 下。

## 渲染后端配置

在游戏资源根的 `setup.env`（`assets/setup.env` 或 `assets/files/setup.env`）设置：

```text
set SEXY_VIDEO_DRIVER=dx11
```

可选 `auto`（默认）、`dx11`、`dx12`、`gl`、`vk`、`metal`，修改后重启。指定后端不可用或着色器包缺失会报告启动失败。Windows 已验证 DX11、DX12、GL、Vulkan；Metal 分段已打包。窗口连接通过 SDL 属性支持 Windows、Cocoa、X11 和 Wayland，Linux/macOS 尚未实机验证，游戏还原代码仍有独立的 32 位布局假设。配置示例见 [setup.env.example](setup.env.example)。

显卡调试层与性能标记默认关闭，包括 C++ Debug 构建。需要检查 GPU 调用时，在 `setup.env` 设置 `SEXY_BGFX_DEBUG=1`；需要 GPU 性能标记时设置 `SEXY_BGFX_PROFILE=1`。这些选项会增加驱动和调试器的开销，修改后重启生效。

原直接 OpenGL 后端已移除；`gl` 使用 bgfx 的 OpenGL renderer。`GLImage` 保留原类名，当前绘制目标会路由到 bgfx，其他目标继续使用 `MemoryImage` 软件绘制。

## SDL 平台层

原 `Win32AppDriver`、`Win32File`、`Win32Gamepad` 已替换为 `SDLAppDriver`、`SDLFile`、`SDLGamepad`，未使用的 Win32 窗口和输入实现已删除。窗口、事件、手柄热插拔及震动、音频流、线程、互斥锁、时钟和文件操作使用 SDL3；存档目录由 `SDL_GetPrefPath` 选择，文件路径统一使用 UTF-8，输出先写临时文件再替换原文件。

手柄优先使用 SDL 自带映射；缺少映射的通用 HID 手柄也会枚举并生成基础映射，默认轴 0/1 为左摇杆、轴 2/3 为右摇杆，按钮 0/1/2/3 为 A/B/X/Y。不同设备的布局可以用资源根下的 `gamecontrollerdb.txt` 覆盖，或在 `setup.env` 中指定 `SDL_GAMECONTROLLERCONFIG_FILE`（资源根相对路径或绝对路径），也支持 `SDL_GAMECONTROLLERCONFIG` 映射字符串。连接日志会记录设备名、GUID 和使用的基础映射。已有映射的 Xbox 手柄保持 SDL 的映射，另一只手柄断开时保留仍连接设备的绑定。

应用驱动使用普通 C++ 虚函数和具名字段，不再读取对象偏移或拼接 Win32 虚表。原账号/商店服务使用 `LocalGameCenter`；原 Java 通知和 RPC 服务保持已有的不可用行为。bgfx 获取系统窗口句柄的适配集中在 SDL 窗口属性接口，无直接 Win32 API 调用。现有 Visual Studio 工程中的 `Win32` 仍是 x86 构建配置名称。

## 运行

```text
LawnTV.exe [--assets <dir>] [--width N] [--height N]
           [--frames N]         跑 N 帧后退出
           [--headless]         隐藏 SDL 窗口，仍使用 GPU 绘制
           [--screenshot <f>]   保存最后一帧离屏图像后退出（隐含 headless）
```

默认窗口 1280×720。不传 `--assets` 时，资源根是 **exe 所在目录**下的 `assets`。运行中按 F12 保存离屏缓冲到 `screenshots/shot_NNNN.png`；`--screenshot` 使用同一个 bgfx 离屏目标，不受窗口遮挡或 DPI 影响。例如：`LawnTV.exe --frames 180 --screenshot out.png --headless`。

重型武器模式用左摇杆移动车子，右摇杆向上半区瞄准时自动连续开火，不需要按射击键。

鼠标左键可点击菜单、选卡、收集阳光和金币；选中卡片后点击草坪种植，也可从卡槽拖到草坪松开。右键取消当前卡片、工具或加农炮选择。铲子和花园工具支持点击选择后使用；加农炮点击选择后再点击目标发射。图鉴说明、小游戏列表支持拖动和滚轮，底部操作提示可直接点击。合作模式可用鼠标操作两排卡槽；选卡时先填玩家一的四张卡，再填玩家二。重型武器支持按住左键左右拖动车辆，并自动向上开火。

游戏中可点击上方的“菜单”石头按钮打开暂停菜单，下方“小道具”按钮切换道具栏的显示；花园与智慧树的菜单按钮返回主菜单。选卡界面的商店、图鉴按钮遵循原版解锁条件。商店车尾的上一页、下一页按钮支持鼠标点击翻页。

“小道具”文字使用资源根 `properties/LawnStrings.txt` 中的 `[TOOLS_BUTTON]` 条目。

在资源根的 `setup.env` 可启用以下选项，修改后重启生效：

```text
set LAWN_AUTO_COLLECT=1
set LAWN_HIDE_COVER_LAYER=1
```

`LAWN_AUTO_COLLECT` 自动收集普通阳光、金币、钻石和对战脑，沿用掉落物原有的收集延迟与动画；合作模式阳光保留双人拾取规则。`LAWN_HIDE_COVER_LAYER` 隐藏右侧灌木、前景遮挡和屋顶树杆。两个选项未配置时默认关闭，示例配置已启用，设为 `0` 可关闭相应扩展。

## 许可

原有重建代码按 `vcpkg.json` 所标的 MIT 许可分发。本次鼠标支持包含从 [PlantsVsZombies-AndroidTV](https://github.com/ZombieYetis/PlantsVsZombies-AndroidTV) 移植、改写的触控代码，这些部分使用 GPL-3.0-or-later；包含这些部分的组合程序分发时须遵守 GPL。来源、版权和改动说明见 [ThirdParty/PvZTVTouch/NOTICE.md](ThirdParty/PvZTVTouch/NOTICE.md)，许可证全文见 [ThirdParty/PvZTVTouch/LICENSE](ThirdParty/PvZTVTouch/LICENSE)。

游戏名称、美术、音乐和关卡数据不属于本仓库，使用原版资源需自行遵守权利人的许可。
