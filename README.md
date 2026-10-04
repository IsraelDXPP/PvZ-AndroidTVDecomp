# LawnProjectTV

把《植物大战僵尸》电视 / Android 版里的 `libGameMain.so` 还原成可在 Windows 上编译、运行的 C++ 工程。目标是对照原二进制把类、虚表和字段偏移对齐，再在桌面上接一套窗口、OpenGL 和音频，而不是做一版新的移植。

当前状态是**重建进行中**。工程可以编出 `LawnTV.exe` 并进入帧循环，但大量函数仍是对着反编译结果逐段誊写的，启动期的堆损坏和布局偏差还在查。不要把它当成能完整通关的客户端。

原作版权归 PopCap / EA。本仓库只放重建出来的代码和构建说明，不包含游戏资源。

## 目录

| 路径 | 内容 |
| --- | --- |
| `src/Game` | 游戏逻辑：`LawnApp`、`Board`、植物、僵尸、关卡、对话框 |
| `src/Sexy` | PopCap SexyApp 框架的还原（控件、资源、渲染接口） |
| `src/Platform` | Windows 替代层：GLFW 窗口、桌面 OpenGL 3.3、OpenAL、文件路径 |
| `src/Runtime` | 让 ARM/libstdc++ 反编译结果能在 MSVC x86 上编译的类型和字符串垫片 |
| `src/main.cpp` | Windows 入口和帧循环 |
| `vcpkg.json` | 第三方库清单 |

原 Android 构建从 JNI 进 `Native::BridgeApp`，由 `Sexy::AndroidAppDriver` 驱动。Windows 上由 `WinMain` 自己构造 `LawnApp` 并跑帧循环。

资源路径按 APK 里的 `files/` 来解析。把资源解到可执行文件旁边的 `assets/`（或 `assets/files/`）即可；程序会在发现 `files` 子目录时自动走进去。也可以用 `--assets` 指到别的目录。

## 构建

需要：

- Visual Studio 2022 或更新（工程工具集是 `v145`，即 VS 2026；用 2022 时把工具集改成已安装的版本）
- 带 manifest 模式的 [vcpkg](https://github.com/microsoft/vcpkg)，并集成进 MSBuild
- **Win32（x86）**。类布局按 32 位 ARM 二进制的字段偏移还原，x64 配置虽然写在工程里，但对象大小对不上。

依赖（见 `vcpkg.json`）：GLFW、glad（桌面 GL 3.3）、GLM、libpng、libjpeg-turbo、FreeType、zlib、minizip-ng、bzip2、libogg、libvorbis、OpenAL Soft、nlohmann-json、pugixml、stb。原版用的是 GLES2 和 Audiere，桌面这边保持同一套 GPU 用法（着色器、纹理、FBO），音频换成 OpenAL。

```text
vcpkg install --triplet x86-windows
```

用 Visual Studio 打开 `LawnProjectTV.slnx`，选 **Debug | Win32** 或 **Release | Win32** 生成。产物是仓库根下的：

```text
build\Win32\<Configuration>\LawnTV.exe
```

把 `vcpkg_installed\x86-windows\bin` 里的 DLL（`glfw3.dll`、`OpenAL32.dll`、`ogg.dll`、`vorbis.dll` 等）放到 exe 旁边，再放上资源目录。

## 运行

```text
LawnTV.exe [--assets <dir>] [--width N] [--height N]
           [--frames N]         跑 N 帧后退出
           [--headless]         不显示窗口（仍会建隐藏的 GL 上下文）
           [--screenshot <f>]   计划把最后一帧写到 <f> 后退出
           [--test-pattern]     计划画一张固定图供像素对比；尚未实现
```

默认窗口 800×600。不传 `--assets` 时，资源根是 **exe 所在目录**下的 `assets`，不看当前工作目录。运行中按 F12 计划截背缓冲到 `screenshots/shot_NNNN.png`；截图表面还没接到渲染设备上，所以 `--screenshot` 和 F12 目前写不出图。

## 许可

重建代码按 `vcpkg.json` 所标的 MIT 许可分发。游戏名称、美术、音乐和关卡数据不属于本仓库，使用原版资源需自行遵守权利人的许可。
