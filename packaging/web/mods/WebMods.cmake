# Browser-only side modules. The native and Node qualification hosts keep their
# existing linkage. Archives supply code, not their transitive engine libraries:
# CK/BML/physics must have exactly one instance, in the main module.
set(_web_mod_targets BallanceTAS tas_core tas_game tas_scripting tas_features
    tas_lua_bindings tas_ui tas_lua_runtime LegacyTASRuntime PracticeTools)
foreach(_target IN LISTS _web_mod_targets)
    set_target_properties(${_target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endforeach()
set_target_properties(liblua_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)
foreach(_dependency IN ITEMS lua::lua fmt::fmt)
    get_target_property(_actual ${_dependency} ALIASED_TARGET)
    if(NOT _actual)
        set(_actual ${_dependency})
    endif()
    set_target_properties(${_actual} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endforeach()
function(ballance_web_side_module name)
    set(_stub "${CMAKE_CURRENT_BINARY_DIR}/${name}Web.cpp")
    file(WRITE "${_stub}" "// Code is supplied by the selected plugin archives.\n")
    add_executable(${name}Web EXCLUDE_FROM_ALL "${_stub}")
    set_target_properties(${name}Web PROPERTIES SUFFIX ".wasm" OUTPUT_NAME "${name}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/web-mods")
    # Remove executable-only options inherited from the Node host preset.
    set_property(TARGET ${name}Web PROPERTY LINK_OPTIONS
        "-sSIDE_MODULE=1" "-sASSERTIONS=1" "-fexceptions")
    target_link_options(${name}Web PRIVATE "-Wl,--whole-archive")
    foreach(_archive IN LISTS ARGN)
        target_link_libraries(${name}Web PRIVATE "$<TARGET_FILE:${_archive}>")
        add_dependencies(${name}Web ${_archive})
    endforeach()
    target_link_libraries(${name}Web PRIVATE "-Wl,--no-whole-archive")
endfunction()
ballance_web_side_module(BallanceTAS BallanceTAS tas_core tas_game tas_scripting
    tas_features tas_lua_bindings tas_ui tas_lua_runtime lua::lua fmt::fmt)
ballance_web_side_module(LegacyTASRuntime LegacyTASRuntime)
ballance_web_side_module(PracticeTools PracticeTools)
add_dependencies(PlayerBrowser BallanceTASWeb LegacyTASRuntimeWeb PracticeToolsWeb)
get_target_property(_links PlayerBrowser LINK_LIBRARIES)
list(REMOVE_ITEM _links BallanceTAS LegacyTASRuntime PracticeTools)
set_property(TARGET PlayerBrowser PROPERTY LINK_LIBRARIES "${_links}")
get_target_property(_defs PlayerBrowser COMPILE_DEFINITIONS)
list(REMOVE_ITEM _defs BALLANCE_STATIC_TAS=1 BALLANCE_STATIC_LEGACY_TAS=1 BALLANCE_STATIC_PRACTICE=1)
set_property(TARGET PlayerBrowser PROPERTY COMPILE_DEFINITIONS "${_defs}")
target_sources(PlayerBrowser PRIVATE "${CMAKE_CURRENT_LIST_DIR}/WebModHost.cpp")
target_link_options(PlayerBrowser PRIVATE "-sMAIN_MODULE=2"
    "--js-library=${CMAKE_CURRENT_LIST_DIR}/web-mod-link.js"
    "-sEXPORTED_RUNTIME_METHODS=['FS','IDBFS','ENV','callMain','ccall','requestFullscreen','loadDynamicLibrary','bwModFunction','bwFinalizeMods','bwHasModSymbol','bwModMetadata']")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
# The ABI uses the published Emscripten calling convention and public layouts.
execute_process(COMMAND "${CMAKE_CXX_COMPILER}" -dumpversion
    OUTPUT_VARIABLE _web_emver OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT _web_emver STREQUAL "6.0.13")
    message(FATAL_ERROR "BallanceWeb ABI v1 requires Emscripten 6.0.13")
endif()
set(_web_abi_flags "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CMAKE_CXX_FLAGS_MINSIZEREL}")
if(_web_abi_flags MATCHES "-fpack-struct|-fshort-enums|-fshort-wchar|-fno-rtti|-fno-exceptions|-fwasm-exceptions|-pthread|-m64")
    message(FATAL_ERROR "Compiler flags would change the published BallanceWeb ABI")
endif()
add_custom_target(WebABIHeaders
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/build-sdk.py"
        --engine "${CMAKE_SOURCE_DIR}/Source" --bml "${BALLANCE_BML_SOURCE_DIR}"
        --output "${CMAKE_BINARY_DIR}/web-sdk" VERBATIM)
add_dependencies(PlayerBrowser WebABIHeaders)
file(GLOB _web_abi_contracts CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/abi/*.json")
set(_web_exports "${CMAKE_BINARY_DIR}/web-mods/host-exports.rsp")
set(_web_js_services "${CMAKE_BINARY_DIR}/web-mods/host-exports.js.json")
add_custom_command(OUTPUT "${_web_exports}"
    BYPRODUCTS "${_web_js_services}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/host-exports.py"
        --nm "${CMAKE_NM}" --node "${CMAKE_CROSSCOMPILING_EMULATOR}"
        --build "${CMAKE_BINARY_DIR}" --output "${_web_exports}"
    DEPENDS BallanceTASWeb LegacyTASRuntimeWeb PracticeToolsWeb ${_ballance_static_module_targets}
        BML CK2Static VxMathStatic "${CMAKE_CURRENT_LIST_DIR}/host-exports.py"
        "${CMAKE_CURRENT_LIST_DIR}/abi_contract.py" ${_web_abi_contracts} VERBATIM)
add_custom_target(WebHostExports DEPENDS "${_web_exports}")
add_dependencies(PlayerBrowser WebHostExports)
target_link_options(PlayerBrowser PRIVATE "@${_web_exports}"
    "-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=@${_web_js_services}")
set_property(TARGET PlayerBrowser APPEND PROPERTY LINK_DEPENDS "${_web_exports}")

add_custom_command(TARGET PlayerBrowser POST_BUILD
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/abi_contract.py"
        --wasm "$<TARGET_FILE_DIR:PlayerBrowser>/PlayerBrowser.wasm" VERBATIM)
