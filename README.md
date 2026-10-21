# LawnProjectTV

A C++ decompilation of `libGameMain.so` from the Plants vs. Zombies TV / Android edition, extended with modifications that lean toward the home-console release. Game logic is recovered from the original ARM binary; the platform layer is native, using SDL3 for windows, input, audio, files, and threads, and bgfx for rendering. The build uses CMake, with presets covering Windows, macOS, Linux, and Android shared libraries.

This repository is a decompilation project, not a clean-room port. Functions, fields, constants, and object layouts follow the binary and its decompiler output, and comments cite the addresses they were recovered from. The TV build, however, leaves parts of the console experience stubbed out, so this fork restores that content and keeps a few PC-style options behind settings. Code that has no counterpart in the binary is written by hand and may still be approximate.

The decompilation is still in progress. The project builds `LawnTV.exe`, launches, and enters the frame loop, but many functions remain approximate, and crashes or layout mismatches can still occur. Full playthrough support is not guaranteed.

The original game is copyrighted by PopCap / EA. This repository contains decompiled and reimplemented code and build instructions, without game assets.

## Scope

- **Decompiled base.** `Lawn` holds the game: `LawnApp`, `Board`, plants, zombies, levels, modes, and dialogs. `Sexy` holds the PopCap SexyApp framework: widgets, resources, string handling, and rendering interfaces. Both follow the ARM binary's symbols, offsets, and object layouts; where decompiler output was ambiguous, the closest C++ equivalent is used.
- **Console-oriented restoration.** Where the TV binary stubs or omits console content, this project restores the parts that fit the local game: the twelve achievements, the house and records scene, the leaderboard sign, console menu artwork, trash piles, and house blueprints. See [Console-oriented content](#console-oriented-content).
- **Native platform layer.** SDL3 supplies windows, events, gamepads, audio streams, threads, clocks, file access, and save paths. bgfx renders through Direct3D 11/12, OpenGL, Vulkan, or Metal. The original JNI entry point, Java activity, and Win32 drivers have been replaced.
- **Optional PC-style behavior.** A PC-style low camera, the larger PC seed bank in single-player, and classic level-spawn data are opt-in; the TV layout and wave data remain the default.

## Console-oriented content

**Achievements.** Twelve achievements use the console definitions and unlock conditions: Adventure completion, all 49 plants, 40 Survival flags, ten Peashooters in one level, a win after using every mower, the Sunflower / Wall-nut / Chomper win, five consecutive Versus wins, a Gargantuar, cooperative Wall-nut Bowling, 25,000 coins spent at the shop, a Cherry Bomb that kills ten zombies, and the Tree of Wisdom reaching 100 feet. Unlocks are stored per profile, announced during play, and can also be granted retroactively from existing records. Shared actions in cooperative and Versus play grant both participating profiles. With `-tod`, lowercase `u` at the main menu unlocks all twelve for testing. [docs/achievement-conditions.md](docs/achievement-conditions.md) records each condition, its boundaries, and the original code that establishes it.

**House and records.** The TV `ShowHouseScreen` (`@0x2B344C`) is a two-byte stub; this project implements the scene using the console layout and layer order. The house screen layers five reanimations, applies the profile's selected house style and upgrade level, places achievement decorations at fixed positions, and anchors plant and zombie trash piles to scene tracks; the piles grow with the profile's kill counts. Clicking a pile or pressing the gamepad's X or Y button opens a records dialog, the unlocked decorations can be highlighted and cycled, and clicking the survival button after Adventure starts Survival: Endless. The options dialog also opens the scene. The shop adds a house-blueprint page after Adventure; its Change House action opens a house-chooser dialog and charges 300 coins only after a different house is confirmed.

**Underground achievements view.** The achievements screen scrolls through a tall panel stack with mouse drag, wheel, keyboard, and gamepad input, listing every achievement with a locked or unlocked icon, its title, its description, and a completion count. It opens from the main menu's achievements button and from the options dialog's Achievements entry.

**Console artwork.** `GetConsoleImage` loads and caches console assets by name: achievement icons, hole panels, trash cans, the survival button, the leaderboard sign, and main-menu background replacements. Missing images are skipped rather than treated as fatal. The main menu draws the selectable house on the original `House` track and places the leaderboard sign over it.

**Controller-first prompts.** The help bar displays gamepad glyphs for the current screen; `LAWN_GAMEPAD_MODE` defaults to on, and hotplugged controllers update the prompts.

The console-oriented content was adapted from the open-source [PlantsVsZombies-AndroidTV](https://github.com/ZombieYetis/PlantsVsZombies-AndroidTV) project under GPL-3.0-or-later. See [Licensing](#licensing) and [ThirdParty/PvZTVTouch/NOTICE.md](ThirdParty/PvZTVTouch/NOTICE.md).

## Repository layout

| Path | Contents |
| --- | --- |
| `Lawn` | Game logic: `LawnApp`, `Board`, plants, zombies, levels, modes, and dialogs |
| `Sexy` | Decompiled PopCap SexyApp framework: widgets, resources, strings, and rendering interfaces |
| `Sexy/src/Platform` | SDL3 windows, input, gamepads, audio, files, threads, and bgfx rendering |
| `Sexy/include/Runtime` | Runtime compatibility helpers for the ARM decompilation and `Sexy::String` |
| `TodLib` | `Tod*` helpers: particles, foley, curves, hesitation brackets, and text drawing |
| `Levels` | Standalone TV and classic level data, installed as `properties/levels.xml` |
| `Lawn/src/main.cpp` | SDL3 application callbacks and frame updates |
| `CMakeLists.txt`, `CMakePresets.json` | Cross-platform builds and Debug / Release presets |
| `cmake` | Dependency discovery and Android vcpkg triplets |
| `android` | SDLActivity launcher; Gradle builds the C++ libraries and APK |
| `ios` | iOS app bundle: `Info.plist`, launch storyboard, app icon |
| `vcpkg.json` | Third-party dependency manifest |
| `Tools` | [SCBP shader compiler, packer, and format documentation](Tools/scbp.md) |
| `Shaders` | [Game shader sources and multiplatform SCBP packages](Shaders/README.md) |
| `ThirdParty` | [GPL-3.0-or-later notice and license](ThirdParty/PvZTVTouch/NOTICE.md) for the adapted code |
| `docs` | [Console achievement condition audit](docs/achievement-conditions.md) |

## Runtime overview

### Entry point

The original Android build entered `Native::BridgeApp` through JNI and used `Sexy::AndroidAppDriver`. The current entry point uses SDL3's `SDL_MAIN_USE_CALLBACKS`: `SDL_AppInit` constructs `LawnApp`, `SDL_AppIterate` updates and draws a frame, `SDL_AppEvent` translates window and input events, and `SDL_AppQuit` stops the loading thread and releases resources. SDL manages the platform entry point and outer event loop. No separate Windows / Android main or JNI entry point is required. `SDL_HINT_MAIN_CALLBACK_RATE` controls frame pacing, retaining `SEXY_MAX_FPS` and the intro video's 30 FPS behavior.

Synchronous `Dialog::WaitForResult` retains its modal event pump and reuses the callback entry point's event translation. Android builds produce a shared library loaded by SDLActivity. SDL's ANativeWindow property supplies the window to bgfx. APK setup and asset packaging are described below.

### Logical canvas

Desktop and Android windows use a logical canvas with a fixed height of 720 and a width determined by the window's aspect ratio: 4:3 gives 960x720, 16:9 gives 1280x720, and 21:9 gives 1680x720. Window size determines the final display scale; assets retain their original drawing coordinates. The original 1280x720 region stays centered. Wider windows reveal more on either side; narrower windows crop the sides. Resizing updates the offscreen canvas, center offset, and pointer mapping together. Android uses the entire SDL surface without fixed 16:9 letterboxing.

### Asset roots

Asset paths follow the APK's `files/` structure. Place extracted resources in `assets/` beside the executable, or in `assets/files/`. The program automatically enters a detected `files` subdirectory. Use `--assets` to select another directory.

The local SDL build uses an offline shop and local saves. It does not configure the original publisher's shop RPC endpoints, start the metrics logging thread, or generate `trans_log`. The original asset-root files `manifest.xml`, `metrics.xml`, `updateclient.ini`, `updateService.xml`, and `version` are unused and may be removed. `setup.env` controls local rendering and debugging options. The game resource manifest `properties/resources.xml` still loads images, sounds, and other resources.

The game can read `assets/main.pak` directly without extraction. Even when loose resources use `assets/files/`, packs are loaded from the outer `assets/` directory. With `--assets`, put packs in the selected directory. Supported formats include the TV edition's ZIP-based packs and PC PopCap packs identified by `0xBAC04AC0` with XOR encoding. Archive paths are ASCII case-insensitive and accept `/`, `\\`, and UTF-8 paths.

Archive access is integrated into `p_fopen`, `p_fread`, `p_fseek`, `p_ftell`, `p_fgetc`, `p_fgets`, `p_feof`, and `p_fclose`. Callers do not need to distinguish disk files from packed resources. A ZIP entry's `compression_method` specifies compression: `0` means stored, and `8` means Deflate, decompressed through minizip-ng and zlib. Compression is not inferred from file extensions or sizes. Existing `p_addResource` support is retained; lower numeric priorities are searched first. Write modes open ordinary files only. Supporting both pack formats does not make their assets interchangeable; this project builds on the TV resource set, and the restored console views load additional artwork by name when it is present.

### Language packs

Set `set LAWN_LANGUAGE=zh_cn` in the external `setup.env` to select Simplified Chinese. An omitted or empty value defaults to `en_us`; restart after changing it. Only the selected `assets/<language>.pak` is mounted alongside the main pack, for example `assets/zh_cn.pak` or `assets/en_us.pak`. Language packs use the same directories as the main pack, such as `data/`, `images/`, and `properties/`, to supply fonts, images, and strings. Lookup order is loose files (priority 0), the language pack (5), then `main.pak` (10). A missing language pack leaves existing resources available. Internal language names are normalized to `zh_CN` and `en_US` without requiring a matching system locale. `setup.env` is always read externally; when using `assets/files/`, the outer `assets/setup.env` is also accepted. Text comes from `properties/LawnStrings.txt`. The obsolete channel override table `LawnOEMStrings.txt` is no longer read and may be removed.

### Level data

`Levels/` ships two complete `levels.xml` variants. `levels.tv.xml` keeps the TV wave data, and `levels.pc.xml` uses classic Adventure spawn algorithms with separate first and second playthrough settings. Each file is standalone; install one as `properties/levels.xml`. The build does not generate or copy level data, and both variants retain all TV modes.

### Adventure 1-1 tutorial

In Adventure level 1-1, `skipShopSeedTutorial` controls only that level's power-up tutorial. Skipping it does not mark the tutorial completed in the profile. Explicit `true` skips it; explicit `false` enables it in the first Adventure playthrough's 1-1; an omitted attribute follows the player's completion record. When enabled, the original TV sequence appends every zombie type from regular zombies through imps that can appear on the current lawn rows after the fourth wave's normal zombies. After 500 spawn updates, the game pauses for Crazy Dave's tutorial, so the level needs at least four waves. Mouse left-click, Enter, and the gamepad confirm button advance the dialogue. The setting controls the dialogue, extra zombies, and free power-up demonstration together.

## Building

Requirements:

- CMake 3.25 or newer, Ninja, and a compiler supporting C++17.
- [vcpkg](https://github.com/microsoft/vcpkg). Set `VCPKG_ROOT` to the directory containing `vcpkg.exe` (or `vcpkg`) and `scripts`.
- Windows game code and vcpkg dependencies use MSVC. Install Visual Studio C++ development tools and the Windows SDK. macOS uses Xcode Command Line Tools; Linux uses GCC or Clang.
- Android additionally requires the NDK. Set `ANDROID_NDK_HOME` to its directory.

All presets use `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake` without machine-specific paths. During configuration, vcpkg installs the dependencies in `vcpkg.json` into `vcpkg_installed/<preset>`. Presets have separate install directories, so switching platforms does not remove another platform's dependencies. For an existing build directory, run `cmake --fresh --preset <preset>` to clear cached dependency paths; vcpkg reuses available binary caches. Android chains `$env{ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake`. The game and dependencies share the same ABI, Android API 28, and `c++_shared` runtime.

Dependencies in `vcpkg.json`: SDL3, SDL3_net, bgfx (multithreaded), SDL3_image (PNG, JPEG), GLM, zlib, minizip-ng, bzip2, libogg, libvorbis, libopenmpt, nlohmann-json, pugixml, and stb. SDL3_image handles images first; stb supplies other formats. Vorbis/openmpt decode audio for SDL3 audio streams. Linux enables the `linux-audio` feature by default for SDL's ALSA output. Game fonts use the original bitmap resources and do not require FreeType.

### Presets

Each prefix below has `-debug` and `-release` variants. Configure and build presets have matching names.

| Platform | Preset prefix | Output |
| --- | --- | --- |
| Windows x64 / x86 | `windows-x64` / `windows-x86` | `LawnTV.exe` |
| iOS arm64 (iOS 15+) | `ios-arm64` | `LawnTV` (`.app` assembled by CI) |
| macOS Apple Silicon / Intel | `macos-arm64` / `macos-x64` | `LawnTV` |
| Linux x64 / arm64 | `linux-x64` / `linux-arm64` | `LawnTV` |
| Android arm64-v8a | `android-arm64` | `libmain.so` |
| Android armeabi-v7a | `android-armv7` | `libmain.so` |
| Android x86_64 emulator | `android-x64` | `libmain.so` |

Desktop presets are shown on their matching host OS. Android presets support cross-compilation from all three desktop systems. Linux presets build natively for their corresponding architecture; cross-compiling for arm64 Linux also requires a compiler and sysroot. macOS architectures match their vcpkg triplets and are built separately.

### Desktop builds

Run these commands from the repository root. On Windows, open a matching-architecture **Developer PowerShell / Native Tools Command Prompt** first. Visual Studio can also open the project folder and select a CMake preset.

```powershell
# Windows: adjust VCPKG_ROOT to your installation.
$env:VCPKG_ROOT = 'C:/vcpkg'
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug --parallel
```

```sh
# macOS / Linux: adjust VCPKG_ROOT to your installation.
export VCPKG_ROOT="$HOME/vcpkg"
cmake --preset macos-arm64-release
cmake --build --preset macos-arm64-release --parallel
# On Linux, use linux-x64-debug, linux-x64-release, or another matching preset.
```

Linux requires the window and audio development libraries used by SDL / bgfx. On Ubuntu / Debian, install:

```sh
sudo apt install build-essential cmake ninja-build pkg-config autoconf automake libtool \
  libx11-dev libxft-dev libxext-dev libwayland-dev libxkbcommon-dev \
  libegl1-mesa-dev libgl1-mesa-dev libibus-1.0-dev
```

Outputs are in `build/<preset>/bin/Debug/` or `build/<preset>/bin/Release/`. vcpkg app-local deployment copies required Windows DLLs beside the executable. Builds compile the program without generating or copying assets, resource packs, or shaders. Place game resources in `assets` beside the executable, or select an asset root with `--assets`. Missing assets do not prevent compilation.

### Android shared libraries

```powershell
$env:VCPKG_ROOT = 'C:/vcpkg'
$env:ANDROID_NDK_HOME = 'C:/Android/Sdk/ndk/<installed-version>'
cmake --preset android-arm64-release
cmake --build --preset android-arm64-release --parallel
```

Use the same commands on macOS / Linux, setting environment variables with `export`. Android API 28 (Android 9) is the minimum supported version, specified consistently in Android triplets and presets.

`cmake/triplets/lawn-*-android.cmake` builds SDL3 as a shared library and other third-party libraries as static libraries with PIC enabled. Standalone CMake builds output `libmain.so`, `libSDL3.so`, and the NDK's `libc++_shared.so`.

### Android APK

The complete launcher is integrated in [android](android/README.md). Open that directory in Android Studio, or run:

```powershell
$env:VCPKG_ROOT = 'C:/vcpkg'
cd android
.\gradlew.bat :app:assembleDebug
# Build the native libraries automatically for each selected ABI.
.\gradlew.bat :app:assembleRelease '-PlawnAbis=arm64-v8a,x86_64'
```

Gradle's `externalNativeBuild` invokes the repository-root `CMakeLists.txt`. Dependencies are installed through `VCPKG_ROOT`; each ABI selects its triplet and uses a separate dependency directory. Native libraries are built and packaged automatically without copying them to `jniLibs`. Gradle/CMake outputs, APKs, local assets, and signing files are ignored by Git.

### iOS app

The iOS build targets iOS 15.0 and later on arm64, for iPhone and iPad. It needs a Mac with Xcode, or the CI workflow, which does the whole thing:

```sh
export VCPKG_ROOT="$HOME/vcpkg"
cmake --preset ios-arm64-release
cmake --build --preset ios-arm64-release --parallel
```

`cmake/ios.toolchain.cmake` fixes the SDK (`iphoneos`), the architecture (`arm64`) and the deployment target (15.0). `cmake/triplets/lawn-arm64-ios.cmake` builds the dependencies with the same minimum, so the app runs on any device with iOS 15 or later. The vcpkg toolchain is the outer one and it chainloads the iOS toolchain, the same way the Android NDK toolchain is chained.

The build produces a plain executable. The `.app` bundle is assembled by `.github/workflows/ios.yml`: it copies `ios/Info.plist`, compiles `ios/LaunchScreen.storyboard` with `ibtool`, builds the app icon from `ios/Assets.xcassets` with `actool`, signs the bundle ad-hoc and zips it into an unsigned `.ipa` for Sideloadly or AltStore. The bundle carries the game resources in `assets/`; the workflow downloads them when the `GAME_ASSETS_URL` repository variable points to a zip, because the assets are not part of this repository.

`Sexy/src/Platform/Bgfx/BgfxRenderDevice.ios.mm` returns the CAMetalLayer that bgfx's iOS backend renders into, which is the same handle bgfx's own examples pass in `bgfx::PlatformData::nwh`. The Objective-C++ helper is the only Apple-specific source in the project.

Place resources in `android/app/src/main/assets/assets/`. Gradle does not copy resources from other directories or generate or copy shaders. Existing resources use standard asset packaging with `noCompress`; builds still work without resources. On first launch, APK assets are installed in the private data directory and `assets.lock` is written on success. `main.pak` is then read from memory. Saves use the application data directory selected by `SDL_GetPrefPath`.

### Shader compiler tools

Normal builds use the repository's pregenerated SCBP packages, which must be placed in the asset directory manually. To regenerate them, add `-DVCPKG_MANIFEST_FEATURES=shader-tools` when configuring CMake. This feature builds bgfx tools for the host; Android does not compile shaderc into the target library. See [Shaders/README.md](Shaders/README.md) for generation commands. Private local overrides can go in the untracked `CMakeUserPresets.json`.

## Source formatting

Source and configuration files use four-space indentation without tabs. `.editorconfig` supplies editor settings, and `.clang-format` supplies C++ formatting rules. Project headers use standard include guards. Classes use natural alignment, and dependencies are declared through CMake. Windows builds enable MSVC `/permissive-` to check standard C++ syntax.

Run `clang-format -i <file>` from the repository root to format a C++ file. All project documentation, comments, configuration descriptions, diagnostics, and built-in fallback text must be in English. Localized resources are maintained separately.

## Renderer configuration

Set the renderer in the game asset root's `setup.env` (`assets/setup.env` or `assets/files/setup.env`):

```text
set SEXY_VIDEO_DRIVER=dx11
```

Choices are `auto` (default), `dx11`, `dx12`, `gl`, `vk`, and `metal`. Restart after changing the setting. An unavailable renderer or missing shader package causes a startup error. Windows DX11, DX12, GL, and Vulkan have been verified; Metal segments are included. SDL window properties support Windows, Cocoa, X11, Wayland, and Android. See [setup.env.example](setup.env.example) for configuration examples.

GPU validation and profiling are disabled by default, including in C++ Debug builds. Set `SEXY_BGFX_DEBUG=1` in `setup.env` for GPU validation or `SEXY_BGFX_PROFILE=1` for GPU profiling markers. These options add driver and debugger overhead and require a restart.

The former direct OpenGL backend has been removed. `gl` selects bgfx's OpenGL renderer. `GLImage` retains its original class name; drawing to the active render target routes through bgfx, while other targets still use `MemoryImage` software rendering.

## SDL platform layer

The original `Win32AppDriver`, `Win32File`, and `Win32Gamepad` have been replaced with `SDLAppDriver`, `SDLFile`, and `SDLGamepad`. Unused Win32 window and input implementations have been removed. SDL3 handles windows, events, gamepad hotplugging and rumble, audio streams, threads, mutexes, clocks, and file operations. `SDL_GetPrefPath` selects the save directory. Paths use UTF-8, and output is written to temporary files before replacing originals.

Gamepads use SDL's mappings first. Generic HID devices without mappings are also enumerated and receive a basic mapping: axes 0/1 for the left stick, axes 2/3 for the right stick, and buttons 0/1/2/3 for A/B/X/Y. Override device layouts with `gamecontrollerdb.txt` in the asset root, or set `SDL_GAMECONTROLLERCONFIG_FILE` in `setup.env` to an absolute or asset-root-relative path. `SDL_GAMECONTROLLERCONFIG` accepts an SDL mapping string. Connection logs include device names, GUIDs, and generated mappings. Xbox controllers with existing SDL mappings keep them, and disconnecting another controller preserves the remaining device's binding.

The application driver uses ordinary C++ virtual functions and named fields without raw object offsets or assembled Win32 vtables. `LocalGameCenter` supplies the original account/shop service interface. Original Java notification and RPC services retain their existing unavailable behavior. bgfx obtains native window handles through SDL window properties without direct Win32 API calls.

## Running

```text
LawnTV.exe [--assets <dir>] [--width N] [--height N]
           [--frames N]         Exit after N frames
           [--headless]         Hide the SDL window while retaining GPU rendering
           [--screenshot <f>]   Save the final offscreen frame and exit (implies headless)
           [-tod]               Enable game debug shortcuts in Debug and Release
```

The default window size is 1280x720. Without `--assets`, resources are loaded from `assets` beside the executable. F12 saves the offscreen buffer to `screenshots/shot_NNNN.png`. `--screenshot` reads the same bgfx offscreen target and is unaffected by occlusion or DPI. Example: `LawnTV.exe --frames 180 --screenshot out.png --headless`.

Use `LawnTV.exe -tod` or `--debug-keys` to enable debug shortcuts. Lowercase `u` at the main menu marks Adventure completed, unlocks game modes and all 12 achievements, and saves the profile. Lowercase `l` during gameplay opens the level-skip dialog. Debug shortcuts remain disabled without the command-line flag.

Incompatible save versions are automatically reset and overwritten. Player profiles retain their names and identities, but progress, gardens, records, and old level-continuation files are cleared and the current version is written immediately. An incompatible user-list version rebuilds the list. A level-continuation version or pointer-size mismatch restarts the current level. After the intro and seed selection, starting gameplay writes the current format without showing the old incompatible-save dialog.

In Heavy Weapon, the left stick moves the vehicle. Aiming the right stick into the upper half-plane fires continuously without a separate fire button.

After completing Adventure, Credits on the main menu first plays the "Zombies on Your Lawn" music video, then the credits, with replay support. Profiles that have not completed Adventure go directly to the credits. Lyrics are centered on the animation origin; lip synchronization and bobbing heads use the original 268 timing entries.

Mouse left-click operates menus, selects seeds, and collects sun and coins. Select a seed and click the lawn to plant it, or drag it from the seed bank and release on the lawn. Right-click cancels the current seed, tool, or Cob Cannon selection. The shovel and garden tools support click-to-select use; select a Cob Cannon and click a target to fire. Almanac descriptions and minigame lists support dragging and the mouse wheel, and bottom action prompts are clickable. In cooperative mode, the mouse can operate both seed banks; seed selection fills player one's four slots before player two's. Heavy Weapon supports holding the left button to drag the vehicle horizontally while firing upward automatically.

During gameplay, click the upper Menu stone button to pause, or the Power-Ups button beneath it to toggle the tool bank. In the Zen Garden and Tree of Wisdom, Menu returns to the main menu. Store and Almanac buttons in seed selection follow the original unlock conditions. Previous/next buttons on the shop truck support mouse navigation. The main menu's house and achievements entries open the house scene and the underground achievements view.

The Power-Ups label uses `[TOOLS_BUTTON]` in the asset root's `properties/LawnStrings.txt`.

Enable gameplay options in the asset root's `setup.env`; restart after changes:

```text
set LAWN_AUTO_COLLECT=1
set LAWN_HIDE_COVER_LAYER=1
set LAWN_LOW_VIEW=1
```

`LAWN_AUTO_COLLECT` automatically collects regular sun, coins, diamonds, and Versus brains using the original collection delays and animations. Cooperative sun retains its two-player pickup rules, and final seed-packet rewards require manual collection. `LAWN_HIDE_COVER_LAYER` hides the right-side bushes, foreground cover, and roof tree/pole. Both default to off when omitted; the example enables them. Set either to `0` to disable it.

`LAWN_LOW_VIEW` enables a PC-style low camera. Following OneCLib's `(460, 60, 820, 600)` background crop, it scales the lawn uniformly to a height of 720 and keeps it centered. The window's aspect ratio still determines the visible horizontal range. Single-player stages use the larger PC-style seed bank and shovel. The seed bank, visible tools, and menu form one horizontally centered group aligned to the top edge. If the row does not fit, the menu moves below the seed bank while the group remains centered. Selected seed cards, flight animations, ready flashes, tutorial arrows, and mouse hit-testing use the same interface coordinates. Two-player modes retain the TV dual-bank layout. The option defaults to off; `0` restores the TV camera. The Zen Garden, Tree of Wisdom, and intro demonstrations retain their original view.

The alternate OGG music path is selected with `LAWN_MUSIC2=1`; per-file tracks require the corresponding audio resources. `LAWN_NO_ATLAS=1` loads individual images instead of sprite-atlas maps and requires a complete set of loose image resources.

## Licensing

The decompiled code is distributed under the MIT license identified in `vcpkg.json`. Console-oriented features and part of the mouse controls include code adapted from [PlantsVsZombies-AndroidTV](https://github.com/ZombieYetis/PlantsVsZombies-AndroidTV), licensed under GPL-3.0-or-later. Distribution of a combined program containing these parts must comply with the GPL. See [ThirdParty/PvZTVTouch/NOTICE.md](ThirdParty/PvZTVTouch/NOTICE.md) for source, copyright, and adaptation details, and [ThirdParty/PvZTVTouch/LICENSE](ThirdParty/PvZTVTouch/LICENSE) for the license text.

The game name, artwork, music, and level data are not covered by this repository. Use original assets in accordance with their owners' permissions.
