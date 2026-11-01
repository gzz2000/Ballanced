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
    "-sEXPORTED_RUNTIME_METHODS=['FS','IDBFS','ENV','callMain','ccall','requestFullscreen','loadDynamicLibrary','bwModFunction','bwFinalizeMods']")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(_web_exports "${CMAKE_BINARY_DIR}/web-mods/host-exports.rsp")
add_custom_command(OUTPUT "${_web_exports}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/host-exports.py"
        --nm "${CMAKE_NM}" --node "${CMAKE_CROSSCOMPILING_EMULATOR}"
        --build "${CMAKE_BINARY_DIR}" --output "${_web_exports}"
    DEPENDS BallanceTASWeb LegacyTASRuntimeWeb PracticeToolsWeb ${_ballance_static_module_targets}
        BML CK2Static VxMathStatic "${CMAKE_CURRENT_LIST_DIR}/host-exports.py" VERBATIM)
add_custom_target(WebHostExports DEPENDS "${_web_exports}")
add_dependencies(PlayerBrowser WebHostExports)
target_link_options(PlayerBrowser PRIVATE "@${_web_exports}")
set_property(TARGET PlayerBrowser APPEND PROPERTY LINK_DEPENDS "${_web_exports}")
