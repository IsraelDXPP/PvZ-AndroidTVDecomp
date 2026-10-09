find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_net CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(minizip-ng CONFIG REQUIRED)
find_package(Ogg CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)
find_package(libopenmpt CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Stb REQUIRED)

add_library(LawnTVDependencies INTERFACE)
target_include_directories(LawnTVDependencies SYSTEM INTERFACE "${Stb_INCLUDE_DIR}")
target_link_libraries(LawnTVDependencies INTERFACE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_net::SDL3_net
    bgfx::bgfx
    ZLIB::ZLIB
    BZip2::BZip2
    MINIZIP::minizip-ng
    Ogg::ogg
    Vorbis::vorbisfile
    libopenmpt::libopenmpt
    pugixml::pugixml
    glm::glm
    nlohmann_json::nlohmann_json
)
if(WIN32)
    target_link_libraries(LawnTVDependencies INTERFACE dbghelp)
endif()

if(ANDROID)
    get_target_property(lawn_sdl_type SDL3::SDL3 TYPE)
    if(NOT lawn_sdl_type STREQUAL "SHARED_LIBRARY")
        message(FATAL_ERROR
            "Android requires shared SDL3 for SDLActivity. Use the lawn-*-android triplets."
        )
    endif()
endif()
