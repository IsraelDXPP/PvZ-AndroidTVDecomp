# vcpkg supplies the dependencies for the iOS build. The vcpkg toolchain is the
# outer one and it chainloads cmake/ios.toolchain.cmake, which fixes the SDK,
# the architecture and the iOS 15.0 deployment target. This mirrors what
# cmake/AndroidVcpkg.cmake does for the Android NDK.
if(NOT VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "lawn-arm64-ios" CACHE STRING "iOS dependency triplet")
endif()
if(NOT VCPKG_OVERLAY_TRIPLETS)
    set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_LIST_DIR}/triplets" CACHE STRING "Local dependency triplets")
endif()
if(NOT VCPKG_INSTALLED_DIR)
    # One install tree per iOS preset, so a simulator build cannot remove the
    # device build's dependencies.
    set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_LIST_DIR}/../vcpkg_installed/ios"
        CACHE PATH "iOS dependency install directory")
endif()

if(NOT CMAKE_TOOLCHAIN_FILE MATCHES "[/\\]vcpkg\\.cmake$")
    if("$ENV{VCPKG_ROOT}" STREQUAL "")
        message(FATAL_ERROR "Set VCPKG_ROOT to your vcpkg directory before building iOS")
    endif()
    if(NOT VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
        set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/ios.toolchain.cmake"
            CACHE FILEPATH "iOS toolchain")
    endif()
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain" FORCE)
endif()
