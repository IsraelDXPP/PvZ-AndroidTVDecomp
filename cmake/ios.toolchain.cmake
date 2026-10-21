# Toolchain para compilar el juego con Xcode para un iPhone/iPad real (arm64).
#
#   cmake -S . -B build-ios -G Ninja \
#     -DCMAKE_TOOLCHAIN_FILE="$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" \
#     -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
#     -DVCPKG_TARGET_TRIPLET=lawn-arm64-ios \
#     -DCMAKE_BUILD_TYPE=Release
#
# Los presets ios-arm64-* ya pasan todo esto; el fichero se puede usar a mano
# cuando se compila fuera de los presets.
#
# Fija iOS 15.0 como minimo (el objetivo del proyecto) y el SDK iphoneos.
# El empaquetado (.app, storyboard, iconos, firma ad-hoc, .ipa) lo hace
# .github/workflows/ios.yml, no CMake.
set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_SYSROOT iphoneos)
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CMAKE_OSX_DEPLOYMENT_TARGET 15.0)

# El binario se firma ad-hoc en el empaquetado, no en la compilacion.
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
