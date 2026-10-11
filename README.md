# LawnProjectTV

把《植物大战僵尸》电视 / Android 版里的 `libGameMain.so` 还原成使用 CMake 构建的 C++ 工程。游戏逻辑对照原二进制还原，平台层使用 SDL3 窗口、输入和音频，以及 bgfx 渲染。预设覆盖 Windows、macOS、Linux 和 Android 原生共享库。

当前状态是**重建进行中**。工程可以编出 `LawnTV.exe` 并进入帧循环，但大量函数仍是对着反编译结果逐段誊写的，启动期的堆损坏和布局偏差还在查。不要把它当成能完整通关的客户端。

原作版权归 PopCap / EA。本仓库只放重建出来的代码和构建说明，不包含游戏资源。

## 目录

| 路径 | 内容 |
| --- | --- |
| `Lawn` | 游戏逻辑：`LawnApp`、`Board`、植物、僵尸、关卡、对话框 |
| `Sexy` | PopCap SexyApp 框架的还原（控件、资源、渲染接口） |
| `Sexy/src/Platform` | SDL3 窗口、输入、手柄、音频、文件及 bgfx 渲染 |
| `Sexy/include/Runtime` | ARM 反编译结果的运行库兼容实现和 `Sexy::String` |
| `Lawn/src/main.cpp` | SDL3 应用回调入口和逐帧更新 |
| `CMakeLists.txt`、`CMakePresets.json` | 跨平台构建及各平台 Debug / Release 预设 |
| `cmake` | 依赖查找和 Android vcpkg triplets |
| `android` | SDLActivity 启动工程，Gradle 自动构建 C++ 和打包 APK |
| `vcpkg.json` | 第三方库清单 |
| `Tools` | [SCBP 着色器编译打包脚本与格式说明](Tools/scbp.md) |
| `Shaders` | [游戏着色器源码及多平台 SCBP 包](Shaders/README.md) |

原 Android 构建从 JNI 进 `Native::BridgeApp`，由 `Sexy::AndroidAppDriver` 驱动。当前入口使用 SDL3 的 `SDL_MAIN_USE_CALLBACKS`：`SDL_AppInit` 构造 `LawnApp`，`SDL_AppIterate` 更新并绘制一帧，`SDL_AppEvent` 转换窗口和输入事件，`SDL_AppQuit` 停止加载线程并释放资源。平台入口和外层事件循环由 SDL 管理，无需另写 Windows / Android 的 main 或 JNI 入口；帧率通过 `SDL_HINT_MAIN_CALLBACK_RATE` 设置，保留 `SEXY_MAX_FPS` 和片头 30 FPS 的行为。

同步的 `Dialog::WaitForResult` 仍保留模态等待事件泵，复用回调入口的事件转换逻辑。Android 构建输出供 SDLActivity 加载的共享库，窗口通过 SDL 的 ANativeWindow 属性交给 bgfx；APK 工程和资源打包方式见下方构建说明。

桌面窗口使用固定 720 高的逻辑画布，宽度随窗口比例变化：4:3 对应 960×720，16:9 对应 1280×720，21:9 对应 1680×720。窗口尺寸只决定最终显示倍率，素材仍按原版坐标绘制；原来的 1280×720 区域始终居中，宽屏增加两侧视野，较窄的窗口从两侧裁切。拖动窗口边框时同步更新离屏画布、居中偏移和鼠标映射。

资源路径按 APK 里的 `files/` 来解析。把资源解到可执行文件旁边的 `assets/`（或 `assets/files/`）即可；程序会在发现 `files` 子目录时自动走进去。也可以用 `--assets` 指到别的目录。

本地 SDL 版本使用离线商店和存档，不配置原发行商的商店 RPC 地址，不启动统计日志线程，也不生成 `trans_log`。原资源根的 `manifest.xml`、`metrics.xml`、`updateclient.ini`、`updateService.xml` 和 `version` 不再承担功能，可从资源中移除；`setup.env` 用于本地渲染、调试等配置。游戏资源清单 `properties/resources.xml` 仍用于加载图片、声音等资源。

也可直接使用 `assets/main.pak`，无需解包。即使散文件资源根选中 `assets/files/`，包仍从外层 `assets/main.pak` 加载。使用 `--assets` 时，包放在所指定资源目录的同一位置。支持 TV 版的 ZIP 型包，以及 PC 版以 `0xBAC04AC0` 为标识、使用 XOR 编码的 PopCap 包。包内路径忽略 ASCII 大小写，兼容 `/`、`\` 和 UTF-8 路径。

在外部 `setup.env` 中设置 `set LAWN_LANGUAGE=zh_cn` 可选中文，未配置或留空时默认 `en_us`，修改后重启生效。程序只额外挂载所选语言的 `assets/<语言>.pak`，例如 `assets/zh_cn.pak` 或 `assets/en_us.pak`；语言包中的目录与主包一致，如 `data/`、`images/`、`properties/`，用于补充该语言的字体、图片和字符串。查找顺序是散文件（优先级 0）、语言包（5）、`main.pak`（10），语言包不存在时继续读取已有资源。游戏内部语言名规范化为 `zh_CN`、`en_US`，不依赖系统安装相应 locale。`setup.env` 始终从外部文件读取；使用 `assets/files/` 时也接受外层 `assets/setup.env`。文本统一使用 `properties/LawnStrings.txt`，旧渠道覆盖表 `LawnOEMStrings.txt` 不再读取，可从资源中删除。

包内读取统一接入 `p_fopen`、`p_fread`、`p_fseek`、`p_ftell`、`p_fgetc`、`p_fgets`、`p_feof`、`p_fclose`，调用方无需判断文件来自磁盘还是资源包。ZIP 每个条目的 `compression_method` 指明压缩方式：`0` 为未压缩，`8` 为 Deflate，由 minizip-ng 调用 zlib 解压；不是按扩展名或文件大小猜测。原有 `p_addResource` 保留，优先级数值越小越先查找。写入模式只打开普通文件。TV 包和 PC 包的格式支持不代表两版资源可以互换，运行仍需本项目使用的 TV 资源。

冒险首关的 `skipShopSeedTutorial` 只控制本关的小道具教学，不会将跳过写成存档里的“已完成”。显式设为 `true` 跳过，显式设为 `false` 在首次冒险的 1-1 重新启用，省略属性时按玩家已完成的记录决定。启用时按原 TV 流程，在第四波的常规僵尸后追加普通僵尸至小鬼范围内每种能在当前草坪行出现的僵尸，出怪 500 次更新后暂停并弹出戴夫教学，因此关卡需至少四波。鼠标左键、回车和手柄确认键都可推进对话；开关同时控制教学对话、追加僵尸和免费小道具演示。

## 构建

需要：

- CMake 3.25 或更新、Ninja，以及支持 C++17 的编译器。
- [vcpkg](https://github.com/microsoft/vcpkg)，设置 `VCPKG_ROOT` 环境变量指向包含 `vcpkg.exe`（或 `vcpkg`）及 `scripts` 的目录。
- Windows 的游戏代码和 vcpkg 依赖统一使用 MSVC，需要 Visual Studio 的 C++ 开发工具和 Windows SDK；macOS 使用 Xcode Command Line Tools；Linux 使用 GCC 或 Clang。
- Android 另需 NDK，设置 `ANDROID_NDK_HOME` 环境变量指向 NDK 目录。

所有预设使用 `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`，不绑定本机绝对路径。配置时 vcpkg 按 `vcpkg.json` 自动安装对应平台的依赖到 `vcpkg_installed/<预设名称>`，各预设独立安装，切换平台不会移除另一平台的依赖。已有构建目录需要执行 `cmake --fresh --preset <预设名称>` 以清除旧的依赖路径缓存；vcpkg 会复用可用的二进制缓存。Android 将 `$env{ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake` 作为链式工具链，游戏和依赖使用相同 ABI、Android API 28 和 `c++_shared`。

依赖见 `vcpkg.json`：SDL3、SDL3_net、bgfx（multithreaded）、SDL3_image（PNG、JPEG）、GLM、zlib、minizip-ng、bzip2、libogg、libvorbis、libopenmpt、nlohmann-json、pugixml、stb。SDL3_image 优先解码图像，stb 补充其他格式；Vorbis/openmpt 解码后交给 SDL3 音频流。Linux 默认开启 `linux-audio` feature，提供 SDL 的 ALSA 音频输出。游戏字体使用原资源的位图字体，无需 FreeType。

### 预设

下列每个名称都带 `-debug` 和 `-release` 两种后缀，配置预设和构建预设同名。

| 平台 | 预设名称前缀 | 产物 |
| --- | --- | --- |
| Windows x64 / x86 | `windows-x64` / `windows-x86` | `LawnTV.exe` |
| macOS Apple Silicon / Intel | `macos-arm64` / `macos-x64` | `LawnTV` |
| Linux x64 / arm64 | `linux-x64` / `linux-arm64` | `LawnTV` |
| Android arm64-v8a | `android-arm64` | `libmain.so` |
| Android armeabi-v7a | `android-armv7` | `libmain.so` |
| Android x86_64 模拟器 | `android-x64` | `libmain.so` |

桌面预设按宿主系统显示，Android 预设可在三个桌面系统交叉编译。Linux 预设用于对应架构的本机构建；arm64 Linux 交叉编译还需另行提供编译器和 sysroot。macOS 的架构与 vcpkg triplet 一致，两个架构分别构建。

### 桌面构建

在项目根目录执行。Windows 的命令行需要先打开匹配架构的 **Developer PowerShell / Native Tools Command Prompt**；Visual Studio 可以直接打开项目文件夹并选择 CMake 预设。

```powershell
# Windows；按自己的安装位置设置 VCPKG_ROOT。
$env:VCPKG_ROOT = 'C:/vcpkg'
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug --parallel
```

```sh
# macOS / Linux；按自己的安装位置设置 VCPKG_ROOT。
export VCPKG_ROOT="$HOME/vcpkg"
cmake --preset macos-arm64-release
cmake --build --preset macos-arm64-release --parallel
# Linux 对应使用 linux-x64-debug、linux-x64-release 等预设。
```

Linux 需要 SDL / bgfx 对应的窗口和音频开发库。Ubuntu / Debian 可先安装：

```sh
sudo apt install build-essential cmake ninja-build pkg-config autoconf automake libtool \
  libx11-dev libxft-dev libxext-dev libwayland-dev libxkbcommon-dev \
  libegl1-mesa-dev libgl1-mesa-dev libibus-1.0-dev
```

产物位于 `build/<预设名称>/bin/Debug/` 或 `build/<预设名称>/bin/Release/`。Windows 所需 DLL 由 vcpkg 的 app-local 部署复制到同目录。构建只编译程序，不生成或复制 assets、资源包和 shader。游戏资源自行放入产物旁的 `assets`；也可运行时用 `--assets` 指定资源根。资源存在与否不影响编译。

### Android 共享库

```powershell
$env:VCPKG_ROOT = 'C:/vcpkg'
$env:ANDROID_NDK_HOME = 'C:/Android/Sdk/ndk/<已安装版本>'
cmake --preset android-arm64-release
cmake --build --preset android-arm64-release --parallel
```

macOS / Linux 使用相同命令，环境变量改用 `export` 设置。支持的 Android 最低版本是 API 28（Android 9），在 Android triplet 和预设中统一指定。

`cmake/triplets/lawn-*-android.cmake` 将 SDL3 编为共享库，其余第三方库编为静态库并启用 PIC。独立 CMake 构建仍输出 `libmain.so`、`libSDL3.so` 和 NDK 的 `libc++_shared.so`。

### Android APK

完整启动工程已整合到 [android](android/README.md)。在 Android Studio 中打开该目录，或直接执行：

```powershell
$env:VCPKG_ROOT = 'C:/vcpkg'
cd android
.\gradlew.bat :app:assembleDebug
# 按 ABI 自动构建对应的 native 库。
.\gradlew.bat :app:assembleRelease '-PlawnAbis=arm64-v8a,x86_64'
```

Gradle 的 `externalNativeBuild` 直接调用仓库根 `CMakeLists.txt`，依赖通过 `VCPKG_ROOT` 安装；ABI 自动选择对应 triplet，各 ABI 使用独立的依赖安装目录。so 自动构建和打包，无需复制到 `jniLibs`。Gradle/CMake 产物、APK、本地资源和签名文件均被 Git 忽略。

资源自行放到 `android/app/src/main/assets/assets/`，Gradle 不从其他目录复制资源，也不生成或复制 shader。已有资源按标准 assets 规则以 `noCompress` 打包，缺少资源仍可编译。首次启动将 APK 中的资源安装到私有数据目录，成功后写入 `assets.lock`；`main.pak` 载入内存后读取。存档仍由 `SDL_GetPrefPath` 写入应用数据目录。

### 重新编译着色器工具

普通构建使用仓库已生成的 SCBP 包。需要重新生成时，在配置命令追加 `-DVCPKG_MANIFEST_FEATURES=shader-tools`；该 feature 将 bgfx 工具构建为宿主机程序，Android 不会把 shaderc 编进目标库。生成步骤见 [Shaders/README.md](Shaders/README.md)。本机私有覆盖配置可写入不提交的 `CMakeUserPresets.json`。

## 源码格式

源码及配置统一使用四个空格缩进，不使用 Tab。`.editorconfig` 提供编辑器设置，`.clang-format` 提供 C++ 格式规则。项目头文件使用标准 include guard；类按自然对齐布局，链接依赖通过 CMake 声明。Windows 编译启用 MSVC `/permissive-`，检查标准 C++ 语法。

在项目根目录执行 `clang-format -i 文件路径` 可按统一规则格式化指定 C++ 文件。

## 渲染后端配置


在游戏资源根的 `setup.env`（`assets/setup.env` 或 `assets/files/setup.env`）设置：

```text
set SEXY_VIDEO_DRIVER=dx11
```

可选 `auto`（默认）、`dx11`、`dx12`、`gl`、`vk`、`metal`，修改后重启。指定后端不可用或着色器包缺失会报告启动失败。Windows 已验证 DX11、DX12、GL、Vulkan；Metal 分段已打包。窗口连接通过 SDL 属性支持 Windows、Cocoa、X11、Wayland 和 Android，Linux/macOS/Android 尚未实机验证。配置示例见 [setup.env.example](setup.env.example)。

显卡调试层与性能标记默认关闭，包括 C++ Debug 构建。需要检查 GPU 调用时，在 `setup.env` 设置 `SEXY_BGFX_DEBUG=1`；需要 GPU 性能标记时设置 `SEXY_BGFX_PROFILE=1`。这些选项会增加驱动和调试器的开销，修改后重启生效。

原直接 OpenGL 后端已移除；`gl` 使用 bgfx 的 OpenGL renderer。`GLImage` 保留原类名，当前绘制目标会路由到 bgfx，其他目标继续使用 `MemoryImage` 软件绘制。

## SDL 平台层

原 `Win32AppDriver`、`Win32File`、`Win32Gamepad` 已替换为 `SDLAppDriver`、`SDLFile`、`SDLGamepad`，未使用的 Win32 窗口和输入实现已删除。窗口、事件、手柄热插拔及震动、音频流、线程、互斥锁、时钟和文件操作使用 SDL3；存档目录由 `SDL_GetPrefPath` 选择，文件路径统一使用 UTF-8，输出先写临时文件再替换原文件。

手柄优先使用 SDL 自带映射；缺少映射的通用 HID 手柄也会枚举并生成基础映射，默认轴 0/1 为左摇杆、轴 2/3 为右摇杆，按钮 0/1/2/3 为 A/B/X/Y。不同设备的布局可以用资源根下的 `gamecontrollerdb.txt` 覆盖，或在 `setup.env` 中指定 `SDL_GAMECONTROLLERCONFIG_FILE`（资源根相对路径或绝对路径），也支持 `SDL_GAMECONTROLLERCONFIG` 映射字符串。连接日志会记录设备名、GUID 和使用的基础映射。已有映射的 Xbox 手柄保持 SDL 的映射，另一只手柄断开时保留仍连接设备的绑定。

应用驱动使用普通 C++ 虚函数和具名字段，不再读取对象偏移或拼接 Win32 虚表。原账号/商店服务使用 `LocalGameCenter`；原 Java 通知和 RPC 服务保持已有的不可用行为。bgfx 获取系统窗口句柄的适配集中在 SDL 窗口属性接口，无直接 Win32 API 调用。

## 运行

```text
LawnTV.exe [--assets <dir>] [--width N] [--height N]
           [--frames N]         跑 N 帧后退出
           [--headless]         隐藏 SDL 窗口，仍使用 GPU 绘制
           [--screenshot <f>]   保存最后一帧离屏图像后退出（隐含 headless）
           [-tod]              开启游戏调试快捷键（Debug、Release 均支持）
```

默认窗口 1280×720。不传 `--assets` 时，资源根是 **exe 所在目录**下的 `assets`。运行中按 F12 保存离屏缓冲到 `screenshots/shot_NNNN.png`；`--screenshot` 使用同一个 bgfx 离屏目标，不受窗口遮挡或 DPI 影响。例如：`LawnTV.exe --frames 180 --screenshot out.png --headless`。

用 `LawnTV.exe -tod`（也可使用 `--debug-keys`）开启调试快捷键。在主菜单按小写 `u` 会将当前存档标记为冒险已通关、解锁各模式和全部 12 项成就，并保存；游戏内的小写 `l` 可打开跳关对话框。不带参数时调试快捷键保持关闭。

存档版本不匹配时自动重置并覆盖：玩家资料保留名字和身份，清空游戏进度、花园、记录及旧的关卡续玩文件，立即写入当前版本；用户列表版本不匹配时重新建立列表。关卡续玩文件版本或指针大小不匹配时重新开始当前关卡，完成开场和选卡、正式开始游戏后自动写回当前版本，不再弹出旧版本存档对话框。

重型武器模式用左摇杆移动车子，右摇杆向上半区瞄准时自动连续开火，不需要按射击键。

冒险通关后，主菜单里的“制作人员名单”会先播放《Zombies on Your Lawn》MV，再显示名单，并支持重播；未通关的存档直接显示名单。MV 歌词按动画原点居中，口型与跳动脑袋使用原版的 268 条时序数据。

鼠标左键可点击菜单、选卡、收集阳光和金币；选中卡片后点击草坪种植，也可从卡槽拖到草坪松开。右键取消当前卡片、工具或加农炮选择。铲子和花园工具支持点击选择后使用；加农炮点击选择后再点击目标发射。图鉴说明、小游戏列表支持拖动和滚轮，底部操作提示可直接点击。合作模式可用鼠标操作两排卡槽；选卡时先填玩家一的四张卡，再填玩家二。重型武器支持按住左键左右拖动车辆，并自动向上开火。

游戏中可点击上方的“菜单”石头按钮打开暂停菜单，下方“小道具”按钮切换道具栏的显示；花园与智慧树的菜单按钮返回主菜单。选卡界面的商店、图鉴按钮遵循原版解锁条件。商店车尾的上一页、下一页按钮支持鼠标点击翻页。

“小道具”文字使用资源根 `properties/LawnStrings.txt` 中的 `[TOOLS_BUTTON]` 条目。

在资源根的 `setup.env` 可启用以下选项，修改后重启生效：

```text
set LAWN_AUTO_COLLECT=1
set LAWN_HIDE_COVER_LAYER=1
set LAWN_LOW_VIEW=1
```

`LAWN_AUTO_COLLECT` 自动收集普通阳光、金币、钻石和对战脑，沿用掉落物原有的收集延迟与动画；合作模式阳光保留双人拾取规则。`LAWN_HIDE_COVER_LAYER` 隐藏右侧灌木、前景遮挡和屋顶树杆。两个选项未配置时默认关闭，示例配置已启用，设为 `0` 可关闭相应扩展。

`LAWN_LOW_VIEW` 启用 PC 风格的低视角，按 OneCLib 的 `(460, 60, 820, 600)` 背景取景框，将草坪等比放大到 720 高并保持居中；窗口比例仍决定横向可见范围。单人关卡同时采用放大的 PC 卡槽和铲子，卡槽、可见工具及菜单作为一整组水平居中，顶部贴住窗口边缘；菜单避开卡槽和可见工具，整行放不下时菜单移到卡槽下方，整组仍然居中。选卡界面的已选卡片、飞行动画、刷新闪光、教学箭头和鼠标点击使用相同的界面坐标。双人模式保留 TV 双卡槽布局。此选项默认关闭，设为 `0` 恢复 TV 视角；花园、智慧树和开场演示沿用原视角。

## 许可

原有重建代码按 `vcpkg.json` 所标的 MIT 许可分发。本次鼠标支持包含从 [PlantsVsZombies-AndroidTV](https://github.com/ZombieYetis/PlantsVsZombies-AndroidTV) 移植、改写的触控代码，这些部分使用 GPL-3.0-or-later；包含这些部分的组合程序分发时须遵守 GPL。来源、版权和改动说明见 [ThirdParty/PvZTVTouch/NOTICE.md](ThirdParty/PvZTVTouch/NOTICE.md)，许可证全文见 [ThirdParty/PvZTVTouch/LICENSE](ThirdParty/PvZTVTouch/LICENSE)。

游戏名称、美术、音乐和关卡数据不属于本仓库，使用原版资源需自行遵守权利人的许可。
