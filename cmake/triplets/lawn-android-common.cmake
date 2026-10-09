set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Android)
set(VCPKG_CMAKE_SYSTEM_VERSION 28)
set(VCPKG_HASH_ADDITIONAL_FILES "${CMAKE_CURRENT_LIST_FILE}")
list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS
    "-DANDROID_PLATFORM=android-28"
    "-DANDROID_STL=c++_shared"
    "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
)

# SDLActivity loads libSDL3.so before libmain.so. Other dependencies are linked
# into libmain.so, keeping the APK's native library list small.
if(PORT STREQUAL "sdl3")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
