cmake_minimum_required(VERSION 3.25)

if(DEFINED ANDROID_ABI)
    include(cmake/AndroidVcpkg.cmake)
endif()

# Presets and plain cmake -S/-B builds both read the user's vcpkg installation.
if(NOT CMAKE_TOOLCHAIN_FILE)
    if("$ENV{VCPKG_ROOT}" STREQUAL "")
        message(FATAL_ERROR "Set VCPKG_ROOT to your vcpkg directory before configuring")
    endif()
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain")
endif()

if(CMAKE_HOST_WIN32)
    # Ninja's MSVC header dependency parser needs a stable /showIncludes prefix.
    set(ENV{VSLANG} 1033)
endif()

project(LawnProjectTV VERSION 0.1.0 LANGUAGES C CXX)

if(WIN32)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        message(FATAL_ERROR "Windows builds require MSVC cl. Use a windows-* preset in a Visual Studio developer shell.")
    endif()
    include(cmake/MSVC.cmake)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
if(ANDROID)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build configuration" FORCE)
endif()

include(cmake/Dependencies.cmake)

add_library(LawnTVBuildOptions INTERFACE)
target_compile_features(LawnTVBuildOptions INTERFACE cxx_std_17)
target_compile_definitions(LawnTVBuildOptions INTERFACE LAWNTV_DECOMP)
target_include_directories(LawnTVBuildOptions INTERFACE
    "${PROJECT_SOURCE_DIR}/Sexy/include"
    "${PROJECT_SOURCE_DIR}/Sexy/include/Platform"
    "${PROJECT_SOURCE_DIR}/Sexy/include/Platform/Bgfx"
    "${PROJECT_SOURCE_DIR}/Sexy/include/Platform/Audio"
    "${PROJECT_SOURCE_DIR}/Sexy/include/Runtime"
    "${PROJECT_SOURCE_DIR}/TodLib/include"
    "${PROJECT_SOURCE_DIR}/Lawn/include"
    "${PROJECT_SOURCE_DIR}/Lawn/include/game"
)
if(WIN32)
    target_compile_definitions(LawnTVBuildOptions INTERFACE
        WIN32 _WINDOWS NOMINMAX _CRT_SECURE_NO_WARNINGS
    )
endif()
if(MSVC)
    target_compile_options(LawnTVBuildOptions INTERFACE
        /W3 /permissive- /bigobj /source-charset:utf-8 /we4302 /we4311 /we4312
    )
endif()

# Object libraries include both frameworks in the final link. Sexy and TodLib
# call each other, so this also avoids platform-dependent static archive order.
add_subdirectory(Sexy)
add_subdirectory(TodLib)
add_subdirectory(Lawn)
