cmake_minimum_required(VERSION 3.16)
project(UILO LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(UILO_AUTO_FETCH    "Auto-download missing dependencies via FetchContent" ON)
option(UILO_BUILD_EXAMPLES "Build example programs" ON)

include(FetchContent)

# ---------------------------------------------------------------------------
# SDL3
# ---------------------------------------------------------------------------
find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
    if(UILO_AUTO_FETCH)
        message(STATUS "[UILO] SDL3 not found; fetching from GitHub...")
        set(SDL_SHARED       OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC       ON  CACHE BOOL "" FORCE)
        set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS        OFF CACHE BOOL "" FORCE)
        set(SDL_INSTALL      OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG        release-3.2.10
            GIT_SHALLOW    TRUE
        )
        FetchContent_MakeAvailable(SDL3)
    else()
        message(FATAL_ERROR "SDL3 not found and UILO_AUTO_FETCH is OFF")
    endif()
endif()

# ---------------------------------------------------------------------------
# bgfx (via bgfx.cmake wrapper -> bgfx::bgfx / bgfx::bx / bgfx::bimg)
# ---------------------------------------------------------------------------
find_package(bgfx CONFIG QUIET)
if(NOT bgfx_FOUND)
    if(UILO_AUTO_FETCH)
        message(STATUS "[UILO] bgfx not found; fetching bgfx.cmake from GitHub...")
        set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BGFX_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)
        set(BGFX_INSTALL        OFF CACHE BOOL "" FORCE)
        set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(bgfx
            GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
            GIT_TAG        v1.128.8786-475
        )
        FetchContent_MakeAvailable(bgfx)
    else()
        message(FATAL_ERROR "bgfx not found and UILO_AUTO_FETCH is OFF")
    endif()
endif()

# ---------------------------------------------------------------------------
# Shaders (compiled per-backend by bgfx-shaderc, embedded as headers)
# ---------------------------------------------------------------------------
# bgfxToolUtils provides bgfx_compile_shaders(). It is included automatically
# by bgfxConfig.cmake. For the FetchContent path the wrapper also exposes it.
if(NOT COMMAND bgfx_compile_shaders)
    if(DEFINED bgfx_SOURCE_DIR AND EXISTS "${bgfx_SOURCE_DIR}/cmake/bgfxToolUtils.cmake")
        include("${bgfx_SOURCE_DIR}/cmake/bgfxToolUtils.cmake")
    endif()
endif()

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

add_library(uilo STATIC ${UILO_SRCS})
add_dependencies(uilo uilo_shaders)

target_include_directories(uilo PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/elements"
)

target_include_directories(uilo PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
    "${_SHADER_OUT_DIR}"
)

target_link_libraries(uilo PUBLIC
    SDL3::SDL3
    bgfx::bgfx
    bgfx::bx
    bgfx::bimg
)

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
