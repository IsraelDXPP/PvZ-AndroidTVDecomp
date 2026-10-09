if(NOT ASSET_DESTINATION OR NOT SHADER_SOURCE)
    message(FATAL_ERROR "The asset destination and shader source are required")
endif()
file(MAKE_DIRECTORY "${ASSET_DESTINATION}")
if(ASSET_SOURCE)
    file(REAL_PATH "${ASSET_SOURCE}" asset_source_path)
    file(REAL_PATH "${ASSET_DESTINATION}" asset_destination_path)
    if(NOT asset_source_path STREQUAL asset_destination_path)
        cmake_path(IS_PREFIX asset_source_path "${asset_destination_path}" NORMALIZE destination_inside_source)
        if(destination_inside_source)
            message(FATAL_ERROR "The asset destination cannot be inside LAWN_ASSETS_DIR")
        endif()
        # COPY updates changed files without deleting existing user settings.
        file(COPY "${ASSET_SOURCE}/" DESTINATION "${ASSET_DESTINATION}")
    endif()
endif()
file(COPY
    "${SHADER_SOURCE}/main.vert.scbp"
    "${SHADER_SOURCE}/main.frag.scbp"
    "${SHADER_SOURCE}/solid.frag.scbp"
    "${SHADER_SOURCE}/yuv.frag.scbp"
    DESTINATION "${ASSET_DESTINATION}/shaders"
)
