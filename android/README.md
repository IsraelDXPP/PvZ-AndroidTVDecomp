# Android 构建

本目录是 SDLActivity 启动工程，与桌面程序共用仓库根目录的 CMake 和全部 C++ 源码。Gradle 会构建选定 ABI 的 `libmain.so`，并自动打包 SDL3 与 `libc++_shared.so`，不需要维护 `jniLibs` 中的预编译库。

安卓创建普通无边框 SDL 窗口，不设置 `SDL_WINDOW_FULLSCREEN`，窗口尺寸以实际 Android surface 为准。游戏画布为 1280×720，保持 16:9 等比缩放并居中；宽屏左右均分黑边，较窄的屏幕使用上下黑边。触摸和鼠标坐标映射到同一显示区域，黑边不接受游戏点击。

Activity 启动和重新获得焦点时启用 SDL 的沉浸式系统界面：隐藏状态栏和导航栏，边缘滑动临时显示后自动收起。此设置不改变 native SDL 窗口的全屏标志。

游戏只允许左右两个横屏方向，不进入竖屏；横屏方向切换遵循系统旋转锁定设置。Manifest 与 SDL 窗口使用相同的方向限制。

## 配置

需要 JDK 17、Android SDK 36、NDK r27c 和 CMake 3.25 以上。设置 `VCPKG_ROOT` 指向 vcpkg。NDK 默认由 Android SDK 管理，版本为 `27.2.12479018`；设置 `ANDROID_NDK_HOME` 时使用该路径。SDK 中的 CMake 默认使用 `3.31.6`，自带 Ninja。

`local.properties` 由 Android Studio 生成，也可自行填写。该文件被 Git 忽略：

```properties
sdk.dir=C:/Users/<用户名>/AppData/Local/Android/Sdk
# 可选：使用本机 CMake，版本应与安装版本一致。
cmake.dir=C:/Program Files/CMake
cmake.version=4.3.3
# 使用外部 CMake 时，Ninja 需要加入 PATH。
```

资源由使用者自行放到 `app/src/main/assets/assets/`，包括 `main.pak`、语言 pak、`setup.env` 和着色器。Gradle 只按 Android 的标准 assets 规则打包已有文件，不从仓库或外部目录复制资源，也不生成或复制 shader。编译不要求资源存在。

## 构建

```powershell
$env:VCPKG_ROOT = 'C:/vcpkg'
.\gradlew.bat :app:assembleDebug
.\gradlew.bat :app:assembleRelease
```

macOS / Linux 使用 `export VCPKG_ROOT=...` 和 `sh ./gradlew`。APK 输出到 `app/build/outputs/apk/<debug 或 release>/`。

默认 ABI 为 `arm64-v8a`。可一次指定多个 ABI，或单独构建模拟器版本：

```powershell
.\gradlew.bat :app:assembleRelease '-PlawnAbis=arm64-v8a,armeabi-v7a,x86_64,x86'
.\gradlew.bat :app:assembleDebug -PlawnAbis=x86_64
```

ABI 自动选择 `cmake/triplets/lawn-*-android.cmake`。vcpkg 依赖安装在仓库的 `vcpkg_installed/android/<ABI>/`，CMake 输出由 Gradle 管理并忽略。修改 C++ 后再次构建 APK 就会自动重新编译所需 so。可用 `-PlawnNdkVersion=<版本>`、`-PlawnCmakeVersion=<版本>` 覆盖工具版本。

## 签名与资源

Debug 使用 Android 默认调试签名。Release 签名配置放在本地 `keystore.properties`，格式参见 [keystore.properties.example](keystore.properties.example)；没有此配置时生成未签名 Release APK。签名文件、密码配置、SDK 路径、资源包、so、APK、`.cxx` 和 Gradle 缓存均被忽略。

所有 APK 资源使用 `noCompress`。启动时将资源复制到应用私有 `files/lawn-assets`，全部完成后写入 `assets.lock`；APK 更新后重新安装资源。native 层一次载入 `main.pak` 到内存，之后的资源读取不会反复访问 APK。

`org/libsdl/app` 的 Java 代码与当前 vcpkg SDL3 3.4.14 匹配，包含安卓摇杆轴值范围限制；升级 native SDL 时也应同步其 Android Java 代码。
