cmake_minimum_required(VERSION 3.25)
# Enable OBJCXX upfront so SDL3's subdirectory can use it on macOS (Ninja + OBJCXX
# requires the language to be declared in the parent project, not deferred).
if(APPLE)
    project(UILO LANGUAGES C CXX OBJCXX)
else()
    project(UILO LANGUAGES C CXX)
endif()

# C++26. CMake has no C++26 flag for MSVC yet; its newest MSVC dialect is 23,
# which it spells /std:c++latest -- MSVC's own newest mode, carrying whatever
# C++26 it implements. Move MSVC to 26 once CMake learns a flag for it.
if(MSVC)
    set(CMAKE_CXX_STANDARD 23)
else()
    set(CMAKE_CXX_STANDARD 26)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Examples and tools are for working ON UILO, not for a project that vendors it,
# so they default off when UILO is pulled in with add_subdirectory. Spelled with
# the source-dir comparison rather than PROJECT_IS_TOP_LEVEL, which needs 3.21.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(_UILO_DEV_DEFAULT ON)
else()
    set(_UILO_DEV_DEFAULT OFF)
endif()

option(UILO_BUILD_EXAMPLES "Build example programs" ${_UILO_DEV_DEFAULT})
option(UILO_BUILD_TOOLS    "Build the project generator and other tools" ${_UILO_DEV_DEFAULT})
option(UILO_SHARED        "Build UILO as a shared library" OFF)

if(MSVC)
    # The build uses the static runtime (/MT), and applications built against
    # it link with the same. VALO and SDL3 build inside this project and follow
    # it.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# SDL3 (vendored by VALO in ext/VALO/ext/SDL3, at the release VALO pins)
# Built here rather than from VALO's prebuilt install so it follows this
# project's configuration and MSVC runtime. Built once in a separate directory
# so `./build.sh clean` doesn't rebuild it
# ---------------------------------------------------------------------------
set(UILO_SDL3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/VALO/ext/SDL3")
if(NOT EXISTS "${UILO_SDL3_DIR}")
    message(FATAL_ERROR
        "[UILO] SDL3 not found at ${UILO_SDL3_DIR}\n"
        "Run ./build.sh (from Git Bash on Windows) -- it clones VALO into "
        "ext/VALO and has VALO fetch SDL3 and its other dependencies.")
endif()

set(SDL_SHARED       OFF CACHE BOOL "" FORCE)
set(SDL_STATIC       ON  CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS        OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL      OFF CACHE BOOL "" FORCE)

# Keep SDL in this build tree and retain its exported usage requirements.
# Importing a cached archive loses system libraries and can mix configurations.
add_subdirectory("${UILO_SDL3_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/sdl3")

# ---------------------------------------------------------------------------
# VALO (vendored in ext/VALO, cloned by build.sh; its own build.sh fetches
# SDL3's source, its header-only libraries and the Vulkan SDK into ext/VALO/ext)
# ---------------------------------------------------------------------------
# VALO is UILO's renderer and window: Vulkan where the vendored SDK is present,
# its software rasteriser everywhere. Added as a subproject, it builds no
# examples or tools of its own, and it takes the SDL3 target defined above
# rather than finding a second copy, so exactly one SDL lives in the process.
set(UILO_EXT_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/ext")
set(UILO_VALO_DIR "${UILO_EXT_DIR}/VALO")
if(NOT EXISTS "${UILO_VALO_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "[UILO] vendored VALO not found at ${UILO_VALO_DIR}\n"
        "Run ./build.sh (from Git Bash on Windows) -- it clones VALO into "
        "ext/VALO and fetches its dependencies.")
endif()

set(VALO_SHARED OFF CACHE BOOL "" FORCE)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/PatchValo.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GnomeResize.cmake")
add_subdirectory("${UILO_VALO_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/valo")

# ---------------------------------------------------------------------------
# UILO library
# ---------------------------------------------------------------------------
file(GLOB_RECURSE UILO_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp"
)
if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # macOS: native scroll shim (Objective-C++) — gives us precise trackpad
    # deltas + OS momentum that SDL3's Cocoa backend drops.
    # (OBJCXX language already enabled in project() declaration above.)
    #
    # Note APPLE is true for iOS as well, and AppKit does not exist there. An
    # iOS build takes MacStubs.cpp instead, exactly as Linux and Windows do.
    list(APPEND UILO_SRCS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/platform/MacScroll.mm"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/platform/MacWindow.mm"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/platform/MacFileDialog.mm"
    )
endif()

if(UILO_SHARED)
    set(_UILO_LIB_TYPE SHARED)
else()
    set(_UILO_LIB_TYPE STATIC)
endif()
add_library(uilo ${_UILO_LIB_TYPE} ${UILO_SRCS})

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
)

target_include_directories(uilo PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/ext/stb"
    "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanosvg"
)

# VALO brings SDL3 with it (PUBLIC on its side), so an application only has to
# link `uilo`.
target_link_libraries(uilo PUBLIC VALO::VALO)
if(TARGET uilo_gnome_decor)
    target_link_libraries(uilo PUBLIC uilo_gnome_decor)
    target_compile_definitions(uilo PRIVATE UILO_GNOME_RESIZE_FIX)
endif()

# The one UI program, compiled to SPIR-V at build time and linked in, so a built
# application needs no shader files or tools beside it. The software backend
# runs the same C++ headers directly; ui.vert / ui.frag are the committed valoc
# translations the Vulkan backend compiles.
valo_embed_shaders(uilo SHADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/shaders/ui.vert.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/renderer/shaders/ui.frag.hpp"
)

# Platform deps for UILO's own native code.
if(APPLE)
    target_link_libraries(uilo PUBLIC
        "-framework AppKit"   # the native scroll-wheel monitor and live resize
        "-framework Cocoa"
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework QuartzCore" # CAMetalLayer used by MacWindow.mm
    )
elseif(WIN32)
    target_link_libraries(uilo PUBLIC gdi32 user32)
else()
    target_link_libraries(uilo PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()

if(MSVC)
    # /utf-8 keeps source and execution charsets consistent for the embedded
    # assets. PUBLIC because UILO's headers carry UTF-8 literals too.
    target_compile_options(uilo PUBLIC /utf-8)
    target_compile_options(uilo PRIVATE /W3)
else()
    target_compile_options(uilo PRIVATE -Wall -Wextra)
endif()


if(UILO_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

if(UILO_BUILD_TOOLS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/uilo-new/CMakeLists.txt")
    add_subdirectory(tools/uilo-new)
endif()
