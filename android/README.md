# Android build

This SDLActivity launcher shares the repository-root CMake project and C++ sources with the desktop application. Gradle builds `libmain.so` for each selected ABI and packages SDL3 and `libc++_shared.so` automatically. Prebuilt libraries do not need to be maintained in `jniLibs`.

Android creates a regular borderless SDL window without `SDL_WINDOW_FULLSCREEN`. Window dimensions follow the actual Android surface. Rendering uses the same scaling as desktop windows: the logical canvas stays 720 pixels high, and its width follows the surface's aspect ratio. The original 1280x720 game region remains centered; wider screens reveal more on either side, and narrower screens crop the sides. There is no fixed 16:9 letterboxing. Surface size changes update the canvas, centering, and touch/mouse coordinate mapping together.

Activity startup and focus restoration enable SDL's immersive system UI, hiding status and navigation bars. Edge swipes can reveal them temporarily before they hide again. This does not change the native SDL window's fullscreen flag.

Only the two landscape orientations are allowed. Portrait is disabled, and landscape rotation follows the system rotation lock. The manifest and SDL window use matching orientation restrictions.

## Configuration

Requires JDK 17, Android SDK 36, NDK r27c, and CMake 3.25 or newer. Set `VCPKG_ROOT` to vcpkg. The Android SDK manages NDK `27.2.12479018` by default; `ANDROID_NDK_HOME` overrides its path. The default SDK CMake version is `3.31.6`, with bundled Ninja.

Android Studio generates `local.properties`, or you can create it manually. Git ignores this file:

```properties
sdk.dir=C:/Users/<username>/AppData/Local/Android/Sdk
# Optional: use local CMake and match its installed version.
cmake.dir=C:/Program Files/CMake
cmake.version=4.3.3
# Add Ninja to PATH when using an external CMake installation.
```

Place resources in `app/src/main/assets/assets/`, including `main.pak`, language packs, `setup.env`, and shaders. Gradle packages existing files using Android's standard asset rules. It does not copy resources from the repository or external directories, or generate or copy shaders. Assets are not required for compilation.

## Building

```powershell
$env:VCPKG_ROOT = 'C:/vcpkg'
.\gradlew.bat :app:assembleDebug
.\gradlew.bat :app:assembleRelease
```

On macOS / Linux, use `export VCPKG_ROOT=...` and `sh ./gradlew`. APKs are written to `app/build/outputs/apk/<debug-or-release>/`.

The default ABI is `arm64-v8a`. Select multiple ABIs or build an emulator version:

```powershell
.\gradlew.bat :app:assembleRelease '-PlawnAbis=arm64-v8a,armeabi-v7a,x86_64,x86'
.\gradlew.bat :app:assembleDebug -PlawnAbis=x86_64
```

Each ABI automatically selects `cmake/triplets/lawn-*-android.cmake`. vcpkg dependencies are installed in the repository's `vcpkg_installed/android/<ABI>/`. Gradle manages CMake outputs, which are ignored by Git. Rebuilding an APK after C++ changes recompiles the required shared libraries automatically. Override tool versions with `-PlawnNdkVersion=<version>` or `-PlawnCmakeVersion=<version>`.

## Signing and assets

Debug uses Android's default debug signing key. Put Release signing settings in local `keystore.properties`, following [keystore.properties.example](keystore.properties.example). Without it, Release APKs are unsigned. Signing files, passwords, SDK paths, resource packs, shared libraries, APKs, `.cxx`, and Gradle caches are ignored by Git.

All APK resources use `noCompress`. Startup copies resources to the application's private `files/lawn-assets` directory and writes `assets.lock` only after completion. APK updates reinstall the resources. The native layer loads `main.pak` into memory once, avoiding repeated access to APK assets.

The `org/libsdl/app` Java code matches vcpkg's current SDL3 3.4.14 and includes Android joystick-axis clamping. Update its Android Java code when upgrading native SDL.
