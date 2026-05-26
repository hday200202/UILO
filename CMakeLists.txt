cmake_minimum_required(VERSION 3.16)
project(UILO LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
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
# UILO library
# ---------------------------------------------------------------------------
file(GLOB_RECURSE UILO_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp"
)

add_library(uilo STATIC ${UILO_SRCS})

target_include_directories(uilo PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/elements"
)

target_include_directories(uilo PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
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
