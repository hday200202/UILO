cmake_minimum_required(VERSION 3.16)
# Enable OBJCXX upfront so SDL3's subdirectory can use it on macOS (Ninja + OBJCXX
# requires the language to be declared in the parent project, not deferred).
if(APPLE)
    project(UILO LANGUAGES C CXX OBJCXX)
else()
    project(UILO LANGUAGES C CXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(UILO_AUTO_FETCH    "Auto-download missing SDL3 via FetchContent" ON)
option(UILO_BUILD_EXAMPLES "Build example programs" ON)
option(UILO_SHARED        "Build UILO as a shared library" OFF)

if(MSVC)
    # bgfx's GENie build uses the static runtime (/MT) for Release; the whole
    # project must match or the linker reports LNK2038 runtime mismatches.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)

# ---------------------------------------------------------------------------
# SDL3 (vendored in ext/SDL3, cloned by build.sh at a pinned tag)
# ---------------------------------------------------------------------------
set(UILO_SDL3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/SDL3")
if(NOT EXISTS "${UILO_SDL3_DIR}")
    message(FATAL_ERROR
        "[UILO] vendored SDL3 not found at ${UILO_SDL3_DIR}\n"
        "Run ./build.sh (or build.ps1 on Windows) -- it clones SDL3 "
        "into ext/SDL3 at a pinned release tag.")
endif()

set(SDL_SHARED       OFF CACHE BOOL "" FORCE)
set(SDL_STATIC       ON  CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS        OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL      OFF CACHE BOOL "" FORCE)
add_subdirectory("${UILO_SDL3_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/ext_SDL3")

# ---------------------------------------------------------------------------
# bgfx (vendored: prebuilt static libs under ext/, built once by build.sh)
# ---------------------------------------------------------------------------
# build.sh / build.ps1 shallow-clone bx, bimg, and bgfx as SIBLING directories
# under ext/ (bgfx's build expects ../bx and ../bimg) and run bgfx's own
# GENie+make build exactly once, gated on the output archive existing. CMake
# never builds bgfx; it links the prebuilt Release archives even for Debug
# builds of UILO (you debug UILO, not bgfx -- see BX_CONFIG_DEBUG below).
set(UILO_EXT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext")

if(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(UILO_BGFX_PLATFORM_DIR "osx-arm64")
    else()
        set(UILO_BGFX_PLATFORM_DIR "osx-x64")
    endif()
elseif(WIN32)
    set(UILO_BGFX_PLATFORM_DIR "win64_vs2022")
else()
    set(UILO_BGFX_PLATFORM_DIR "linux64_gcc")
endif()
set(UILO_BGFX_BIN_DIR "${UILO_EXT_DIR}/bgfx/.build/${UILO_BGFX_PLATFORM_DIR}/bin")

if(NOT EXISTS "${UILO_BGFX_BIN_DIR}")
    message(FATAL_ERROR
        "[UILO] prebuilt bgfx not found at ${UILO_BGFX_BIN_DIR}\n"
        "Run ./build.sh (or build.ps1 on Windows) -- it clones bx/bimg/bgfx "
        "into ext/ and builds them once.")
endif()

find_library(UILO_BGFX_LIB        NAMES bgfxRelease        PATHS "${UILO_BGFX_BIN_DIR}" NO_DEFAULT_PATH REQUIRED)
find_library(UILO_BIMG_DECODE_LIB NAMES bimg_decodeRelease PATHS "${UILO_BGFX_BIN_DIR}" NO_DEFAULT_PATH REQUIRED)
find_library(UILO_BIMG_LIB        NAMES bimgRelease        PATHS "${UILO_BGFX_BIN_DIR}" NO_DEFAULT_PATH REQUIRED)
find_library(UILO_BX_LIB          NAMES bxRelease          PATHS "${UILO_BGFX_BIN_DIR}" NO_DEFAULT_PATH REQUIRED)

# ---------------------------------------------------------------------------
# Shaders (compiled per-backend by bgfx-shaderc, embedded as headers)
# ---------------------------------------------------------------------------
# bgfx_compile_shaders comes from cmake/bgfxToolUtils.cmake (vendored from
# bgfx.cmake). It requires the bgfx::shaderc target to exist BEFORE inclusion
# (the whole shader section of that file is gated on it), plus
# BGFX_SHADER_INCLUDE_PATH for shaderc's -i flag. shaderc itself is the
# prebuilt binary from bgfx's --with-tools build.
add_executable(bgfx::shaderc IMPORTED)
set_target_properties(bgfx::shaderc PROPERTIES
    IMPORTED_LOCATION "${UILO_BGFX_BIN_DIR}/shadercRelease${CMAKE_EXECUTABLE_SUFFIX}")
set(BGFX_SHADER_INCLUDE_PATH "${UILO_EXT_DIR}/bgfx/src")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/bgfxToolUtils.cmake")

set(_SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/shaders")
set(_SHADER_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders")
file(MAKE_DIRECTORY "${_SHADER_OUT_DIR}")

# Profiles: spirv (Vulkan), 120 (OpenGL), 300_es (GLES/WebGL2), metal (Apple),
# s_5_0 / s_6_0 (D3D11 / D3D12 on Windows).
set(_UILO_SHADER_PROFILES spirv 120 300_es)
if(APPLE)
    list(APPEND _UILO_SHADER_PROFILES metal)
endif()
if(WIN32)
    list(APPEND _UILO_SHADER_PROFILES s_5_0 s_6_0)
endif()

bgfx_compile_shaders(
    TYPE VERTEX
    SHADERS
        "${_SHADER_SRC_DIR}/vs_solid.sc"
        "${_SHADER_SRC_DIR}/vs_tex.sc"
    VARYING_DEF "${_SHADER_SRC_DIR}/varying.def.sc"
    OUTPUT_DIR  "${_SHADER_OUT_DIR}"
    OUT_FILES_VAR UILO_VS_HEADERS
    INCLUDE_DIRS "${_SHADER_SRC_DIR}"
    PROFILES ${_UILO_SHADER_PROFILES}
    AS_HEADERS
)

bgfx_compile_shaders(
    TYPE FRAGMENT
    SHADERS
        "${_SHADER_SRC_DIR}/fs_solid.sc"
        "${_SHADER_SRC_DIR}/fs_tex.sc"
        "${_SHADER_SRC_DIR}/fs_text.sc"
        "${_SHADER_SRC_DIR}/fs_blur.sc"
        "${_SHADER_SRC_DIR}/fs_glass.sc"
    VARYING_DEF "${_SHADER_SRC_DIR}/varying.def.sc"
    OUTPUT_DIR  "${_SHADER_OUT_DIR}"
    OUT_FILES_VAR UILO_FS_HEADERS
    INCLUDE_DIRS "${_SHADER_SRC_DIR}"
    PROFILES ${_UILO_SHADER_PROFILES}
    AS_HEADERS
)

add_custom_target(uilo_shaders DEPENDS ${UILO_VS_HEADERS} ${UILO_FS_HEADERS})

# ---------------------------------------------------------------------------
# UILO library
# ---------------------------------------------------------------------------
file(GLOB_RECURSE UILO_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp"
)

# macOS: native scroll shim (Objective-C++) — gives us precise trackpad
# deltas + OS momentum that SDL3's Cocoa backend drops.
# (OBJCXX language already enabled in project() declaration above.)
if(APPLE)
    list(APPEND UILO_SRCS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/platform/MacScroll.mm"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/platform/MacWindow.mm"
    )
endif()

if(UILO_SHARED)
    set(_UILO_LIB_TYPE SHARED)
else()
    set(_UILO_LIB_TYPE STATIC)
endif()
add_library(uilo ${_UILO_LIB_TYPE} ${UILO_SRCS})
add_dependencies(uilo uilo_shaders)

set_target_properties(uilo PROPERTIES
    # On Windows, export every symbol from the DLL so consumers don't need
    # __declspec(dllexport) annotations. No-op on other platforms.
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    # macOS: make the dylib relocatable via @rpath so consumers can point at
    # it with their own RPATH instead of an absolute install name.
    MACOSX_RPATH ON
    INSTALL_NAME_DIR "@rpath"
    # Always drop the built library into <UILO>/lib regardless of generator,
    # config, or platform. ARCHIVE = .a/.lib (static or DLL import lib),
    # LIBRARY = .so/.dylib, RUNTIME = .dll on Windows.
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)
# Multi-config generators (MSVC, Xcode) append a per-config subdir unless we
# override each config explicitly.
foreach(_cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(uilo PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY_${_cfg} "${CMAKE_CURRENT_SOURCE_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY_${_cfg} "${CMAKE_CURRENT_SOURCE_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY_${_cfg} "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    )
endforeach()

target_include_directories(uilo PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/elements"
    "${UILO_EXT_DIR}/bgfx/include"
    "${UILO_EXT_DIR}/bimg/include"
    "${UILO_EXT_DIR}/bx/include"
)

# bx needs a per-toolchain compat include dir on macOS and Windows.
if(APPLE)
    target_include_directories(uilo PUBLIC "${UILO_EXT_DIR}/bx/include/compat/osx")
elseif(MSVC)
    target_include_directories(uilo PUBLIC "${UILO_EXT_DIR}/bx/include/compat/msvc")
elseif(MINGW)
    target_include_directories(uilo PUBLIC "${UILO_EXT_DIR}/bx/include/compat/mingw")
endif()

# bx headers static_assert unless every consumer TU defines this: 1 in Debug,
# 0 otherwise. PUBLIC so anything that includes UILO headers gets it too.
target_compile_definitions(uilo PUBLIC "BX_CONFIG_DEBUG=$<IF:$<CONFIG:Debug>,1,0>")

target_include_directories(uilo PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/ext/stb"
    "${_SHADER_OUT_DIR}"
)

# Static link order matters: dependents before dependencies.
# uilo -> SDL3 -> bgfx -> bimg_decode -> bimg -> bx (bimg_decode holds imageParse and
# depends on bimg; bx is always last).
target_link_libraries(uilo PUBLIC
    SDL3-static
    "${UILO_BGFX_LIB}"
    "${UILO_BIMG_DECODE_LIB}"
    "${UILO_BIMG_LIB}"
    "${UILO_BX_LIB}"
)

# Platform deps required by the static bgfx/bx archives.
if(APPLE)
    target_link_libraries(uilo PUBLIC
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
        "-framework AppKit"   # also used directly by the native scroll-wheel monitor
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreVideo"
        "-framework CoreMedia"
        "-framework VideoToolbox"
    )
elseif(WIN32)
    # DirectX headers/libs are vendored inside bgfx -- no DX SDK needed.
    target_link_libraries(uilo PUBLIC gdi32 psapi user32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(uilo PUBLIC Threads::Threads ${CMAKE_DL_LIBS} GL)
endif()

if(MSVC)
    target_compile_options(uilo PRIVATE /W3)
else()
    target_compile_options(uilo PRIVATE -Wall -Wextra)
endif()

# Linux: link X11 (Renderer.cpp pulls in Xlib for the native window handle).
if(UNIX AND NOT APPLE)
    find_package(X11)
    if(X11_FOUND)
        target_link_libraries(uilo PUBLIC ${X11_LIBRARIES})
        target_include_directories(uilo PRIVATE ${X11_INCLUDE_DIR})
    endif()
endif()


if(UILO_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()
