# Triplet de vcpkg para iOS arm64 (iPhone/iPad reales).
#
# Es el triplet builtin arm64-ios con el target minimo fijado a 15.0, que es lo
# que pide el proyecto: las dependencias se compilan con el mismo minimo que el
# juego, asi que un .app firmado en iOS 15 corre en cualquier iOS 15 en adelante.
set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME iOS)
set(VCPKG_CMAKE_SYSTEM_VERSION 15.0)
set(VCPKG_OSX_SYSROOT iphoneos)
set(VCPKG_OSX_DEPLOYMENT_TARGET 15.0)
set(VCPKG_OSX_ARCHITECTURES arm64)
set(VCPKG_HASH_ADDITIONAL_FILES "${CMAKE_CURRENT_LIST_FILE}")
