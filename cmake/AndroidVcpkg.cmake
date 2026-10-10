# Gradle supplies the NDK toolchain and ANDROID_ABI before project(). Route
# that toolchain through vcpkg, just as the standalone Android presets do.
if(NOT VCPKG_TARGET_TRIPLET)
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        set(lawn_android_triplet lawn-arm64-android)
    elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
        set(lawn_android_triplet lawn-armv7-android)
    elseif(ANDROID_ABI STREQUAL "x86_64")
        set(lawn_android_triplet lawn-x64-android)
    elseif(ANDROID_ABI STREQUAL "x86")
        set(lawn_android_triplet lawn-x86-android)
    else()
        message(FATAL_ERROR "Unsupported Android ABI: ${ANDROID_ABI}")
    endif()
    set(VCPKG_TARGET_TRIPLET "${lawn_android_triplet}" CACHE STRING "Android dependency triplet")
endif()
if(NOT VCPKG_OVERLAY_TRIPLETS)
    set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_LIST_DIR}/triplets" CACHE STRING "Local Android triplets")
endif()
if(NOT VCPKG_INSTALLED_DIR)
    # Give each ABI an independent install tree so parallel configurations
    # cannot remove or overwrite one another's dependencies.
    set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_LIST_DIR}/../vcpkg_installed/android/${ANDROID_ABI}"
        CACHE PATH "Android dependency install directory")
endif()

if(NOT CMAKE_TOOLCHAIN_FILE MATCHES "[/\\]vcpkg\\.cmake$")
    if("$ENV{VCPKG_ROOT}" STREQUAL "")
        message(FATAL_ERROR "Set VCPKG_ROOT to your vcpkg directory before building Android")
    endif()
    if(NOT VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
        if(CMAKE_TOOLCHAIN_FILE)
            set(lawn_ndk_toolchain "${CMAKE_TOOLCHAIN_FILE}")
        elseif(DEFINED ANDROID_NDK)
            set(lawn_ndk_toolchain "${ANDROID_NDK}/build/cmake/android.toolchain.cmake")
        elseif(NOT "$ENV{ANDROID_NDK_HOME}" STREQUAL "")
            set(lawn_ndk_toolchain "$ENV{ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake")
        else()
            message(FATAL_ERROR "An Android NDK toolchain is required")
        endif()
        set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${lawn_ndk_toolchain}" CACHE FILEPATH "Android NDK toolchain")
    endif()
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain" FORCE)
endif()

# vcpkg's Android toolchain reads this environment variable. Use the NDK
# selected by Gradle even when it was found through the Android SDK instead.
if(VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
    get_filename_component(lawn_ndk_cmake_directory "${VCPKG_CHAINLOAD_TOOLCHAIN_FILE}" DIRECTORY)
    get_filename_component(lawn_ndk_directory "${lawn_ndk_cmake_directory}/../.." ABSOLUTE)
    set(ENV{ANDROID_NDK_HOME} "${lawn_ndk_directory}")
endif()
