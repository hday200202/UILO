# libdecor 0.2.5 redraws each queued configure synchronously, which stalls
# GNOME live resizing at high DPI. Build a private, ABI-compatible core with
# configure coalescing; continue using the system's GTK decoration plugin.
option(UILO_GNOME_RESIZE_FIX "Coalesce GNOME resize events with libdecor 0.2.5" ON)
if(NOT UILO_GNOME_RESIZE_FIX OR NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    return()
endif()
find_package(PkgConfig QUIET)
if(NOT PkgConfig_FOUND)
    return()
endif()
pkg_check_modules(UILO_SYSTEM_DECOR QUIET libdecor-0=0.2.5)
if(NOT UILO_SYSTEM_DECOR_FOUND)
    return()
endif()
pkg_check_modules(UILO_WAYLAND REQUIRED IMPORTED_TARGET wayland-client)
find_program(UILO_WAYLAND_SCANNER wayland-scanner REQUIRED)
pkg_get_variable(_uilo_decor_libdir libdecor-0 libdir)

include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
FetchContent_Declare(uilo_decor_source
    URL https://gitlab.freedesktop.org/libdecor/libdecor/-/archive/149c6f0b05663aaa69fdf7f94be2483776d1a311/libdecor-149c6f0b05663aaa69fdf7f94be2483776d1a311.tar.gz
    URL_HASH SHA256=cec23d05f4046e2a319643076615cfb39caafcc000a568f887df72e28533b603
)
FetchContent_MakeAvailable(uilo_decor_source)
set(_uilo_decor_patch "${CMAKE_CURRENT_LIST_DIR}/patches/libdecor-gnome-resize.patch")
find_program(UILO_PATCH_TOOL patch REQUIRED)
execute_process(COMMAND "${UILO_PATCH_TOOL}" -p1 --reverse --dry-run --force --input "${_uilo_decor_patch}"
    WORKING_DIRECTORY "${uilo_decor_source_SOURCE_DIR}"
    RESULT_VARIABLE _uilo_decor_patched OUTPUT_QUIET ERROR_QUIET)
if(NOT _uilo_decor_patched EQUAL 0)
    execute_process(COMMAND "${UILO_PATCH_TOOL}" -p1 --forward --batch --input "${_uilo_decor_patch}"
        WORKING_DIRECTORY "${uilo_decor_source_SOURCE_DIR}"
        RESULT_VARIABLE _uilo_decor_result ERROR_VARIABLE _uilo_decor_error)
    if(NOT _uilo_decor_result EQUAL 0)
        message(FATAL_ERROR "[UILO] Could not patch libdecor: ${_uilo_decor_error}")
    endif()
endif()
set(_uilo_decor_build "${CMAKE_CURRENT_BINARY_DIR}/gnome-decor")
file(MAKE_DIRECTORY "${_uilo_decor_build}")
configure_file("${CMAKE_CURRENT_LIST_DIR}/decor-config.h.in"
    "${_uilo_decor_build}/config.h" @ONLY)
configure_file("${uilo_decor_source_SOURCE_DIR}/LICENSE"
    "${_uilo_decor_build}/LICENSE.libdecor" COPYONLY)
set(_uilo_decor_protocols)
foreach(_protocol xdg-shell xdg-decoration)
    if(_protocol STREQUAL "xdg-decoration")
        set(_xml "xdg-decoration-unstable-v1")
    else()
        set(_xml "${_protocol}")
    endif()
    set(_input "${UILO_SDL3_DIR}/wayland-protocols/${_xml}.xml")
    set(_header "${_uilo_decor_build}/${_protocol}-client-protocol.h")
    set(_code "${_uilo_decor_build}/${_protocol}-protocol.c")
    add_custom_command(OUTPUT "${_header}" "${_code}"
        COMMAND "${UILO_WAYLAND_SCANNER}" client-header "${_input}" "${_header}"
        COMMAND "${UILO_WAYLAND_SCANNER}" private-code "${_input}" "${_code}"
        DEPENDS "${_input}" VERBATIM)
    list(APPEND _uilo_decor_protocols "${_header}" "${_code}")
endforeach()
add_library(uilo_gnome_decor SHARED
    "${uilo_decor_source_SOURCE_DIR}/src/libdecor.c"
    "${uilo_decor_source_SOURCE_DIR}/src/libdecor-fallback.c"
    "${CMAKE_CURRENT_LIST_DIR}/decor-anchor.c"
    ${_uilo_decor_protocols})
set_target_properties(uilo_gnome_decor PROPERTIES
    OUTPUT_NAME decor-0 SOVERSION 0 VERSION 0.200.5
    LIBRARY_OUTPUT_DIRECTORY "${_uilo_decor_build}"
    C_VISIBILITY_PRESET hidden)
target_include_directories(uilo_gnome_decor PRIVATE
    "${_uilo_decor_build}" "${uilo_decor_source_SOURCE_DIR}/src")
target_link_libraries(uilo_gnome_decor PRIVATE PkgConfig::UILO_WAYLAND ${CMAKE_DL_LIBS})
message(STATUS "UILO: GNOME live resize fix enabled for libdecor 0.2.5")
