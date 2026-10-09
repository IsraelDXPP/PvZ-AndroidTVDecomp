# MSVC may only have a non-English language pack. Detect its actual include
# prefix with UTF-8 output instead of relying on CMake's locale decoding.
set(lawn_msvc_launcher "cmd.exe;/d;/c;${PROJECT_SOURCE_DIR}/cmake/MsvcUtf8.cmd")
set(lawn_include_probe_dir "${CMAKE_BINARY_DIR}/CMakeFiles/LawnTVIncludeProbe")
file(MAKE_DIRECTORY "${lawn_include_probe_dir}")
file(WRITE "${lawn_include_probe_dir}/LawnTVHeaderProbe.h" "#pragma once\n")
file(WRITE "${lawn_include_probe_dir}/probe.cpp" "#include \"${lawn_include_probe_dir}/LawnTVHeaderProbe.h\"\n")
execute_process(
    COMMAND ${lawn_msvc_launcher} "${CMAKE_CXX_COMPILER}" /nologo /showIncludes /c /Foprobe.obj probe.cpp
    WORKING_DIRECTORY "${lawn_include_probe_dir}"
    RESULT_VARIABLE lawn_include_probe_result
    OUTPUT_VARIABLE lawn_include_probe_output
    ERROR_VARIABLE lawn_include_probe_error
    ENCODING UTF-8
)
if(NOT lawn_include_probe_result EQUAL 0)
    message(FATAL_ERROR "Unable to detect MSVC's include prefix: ${lawn_include_probe_error}")
endif()
string(APPEND lawn_include_probe_output "${lawn_include_probe_error}")
string(REGEX MATCH "(^|[\r\n])([^\r\n]*)[A-Za-z]:[/\\][^\r\n]*LawnTVHeaderProbe\\.h"
    lawn_include_probe_line "${lawn_include_probe_output}")
if(NOT lawn_include_probe_line)
    message(FATAL_ERROR "MSVC did not report the probe header: ${lawn_include_probe_output}")
endif()
set(CMAKE_CL_SHOWINCLUDES_PREFIX "${CMAKE_MATCH_2}")
set(CMAKE_CXX_COMPILER_LAUNCHER "${lawn_msvc_launcher}")
