# Include this from a standalone Emscripten project, using a released SDK folder.
include(CMakeParseArguments)
function(ballance_web_mod target)
    cmake_parse_arguments(MOD "" "ID;VERSION" "SOURCES;REQUIRES;RESOURCES" ${ARGN})
    if(NOT EMSCRIPTEN OR NOT EXISTS "${BALLANCE_WEB_SDK}/sdk.json")
        message(FATAL_ERROR "Use emcmake and set BALLANCE_WEB_SDK to the matching SDK")
    endif()
    if(NOT MOD_ID MATCHES "^[A-Za-z][A-Za-z0-9_]*$" OR NOT MOD_VERSION)
        message(FATAL_ERROR "Supply ID (C identifier) and VERSION")
    endif()
    execute_process(COMMAND "${CMAKE_CXX_COMPILER}" -dumpversion OUTPUT_VARIABLE _emver OUTPUT_STRIP_TRAILING_WHITESPACE)
    file(READ "${BALLANCE_WEB_SDK}/sdk.json" _sdk)
    string(JSON _expected GET "${_sdk}" emscripten)
    if(NOT _emver STREQUAL _expected)
        message(FATAL_ERROR "SDK requires Emscripten ${_expected}, got ${_emver}")
    endif()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_executable(${target} ${MOD_SOURCES})
    set_target_properties(${target} PROPERTIES SUFFIX ".wasm" POSITION_INDEPENDENT_CODE ON)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_include_directories(${target} PRIVATE "${BALLANCE_WEB_SDK}/include")
    target_compile_options(${target} PRIVATE -fexceptions -ffp-contract=off)
    target_compile_definitions(${target} PRIVATE
        BMLEntry=BW_${MOD_ID}_Entry BMLExit=BW_${MOD_ID}_Exit
        BWMOD_RESOURCE_ROOT="/web-mods/${MOD_ID}"
        IMGUI_USER_CONFIG="BMLImGuiConfig.h")
    target_link_options(${target} PRIVATE -sSIDE_MODULE=1 -fexceptions)
    set(_extra)
    foreach(_dep IN LISTS MOD_REQUIRES)
        list(APPEND _extra --requires "${_dep}")
    endforeach()
    set(_resource_deps)
    foreach(_resource IN LISTS MOD_RESOURCES)
        list(APPEND _extra --resource "${_resource}")
        string(REGEX REPLACE "=.*$" "" _source "${_resource}")
        list(APPEND _resource_deps "${_source}")
    endforeach()
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${MOD_ID}.bwmod"
        COMMAND "${Python3_EXECUTABLE}" "${BALLANCE_WEB_SDK}/pack-mod.py"
            --sdk "${BALLANCE_WEB_SDK}/sdk.json" --id "${MOD_ID}" --version "${MOD_VERSION}"
            --module "$<TARGET_FILE:${target}>" --create BW_${MOD_ID}_Entry --destroy BW_${MOD_ID}_Exit
            ${_extra} --output "${CMAKE_CURRENT_BINARY_DIR}/${MOD_ID}.bwmod"
        DEPENDS ${target} ${_resource_deps} "${BALLANCE_WEB_SDK}/sdk.json"
            "${BALLANCE_WEB_SDK}/pack-mod.py" VERBATIM)
    add_custom_target(${target}Package ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${MOD_ID}.bwmod")
endfunction()
