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

# Web backend. UILO's element tree is renderer-free to construct -- VALO is only
# touched from update()/render(), which the web backend never calls. With this
# ON we swap one rendering target for another: the renderer and platform sources
# are dropped, a no-op backend takes their place, and Wt is built and linked
# instead of VALO. The UI code you write is identical either way.
option(UILO_WT "Build UILO for the web (Wt) instead of VALO" OFF)

if(UILO_WT)
    set(UILO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(UILO_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)
    message(STATUS "[UILO] UILO_WT: web build -- Wt instead of VALO")
endif()

if(MSVC AND NOT UILO_WT)
    # The desktop build has always used the static runtime (/MT), and
    # applications built against it link with the same. VALO and SDL3 build
    # inside this project and follow it. The web (UILO_WT) build's Boost is
    # built with runtime-link=shared (/MD), so it stays on the default dynamic
    # runtime instead -- forcing /MT there would mismatch Boost.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# Wt + Boost (vendored in ext/wt and ext/boost, provisioned by `build.sh wt`)
# ---------------------------------------------------------------------------
# The web backend's equivalent of the SDL3/VALO section below: same pattern,
# same ext/ directory, cloned and built once by build.sh.
if(UILO_WT)
    set(UILO_WT_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/ext/wt")
    set(UILO_BOOST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/boost")

    if(NOT EXISTS "${UILO_WT_DIR}/CMakeLists.txt" OR NOT EXISTS "${UILO_BOOST_DIR}")
        message(FATAL_ERROR
            "[UILO] vendored Wt/Boost not found under ${CMAKE_CURRENT_SOURCE_DIR}/ext\n"
            "Run ./build.sh wt (from Git Bash on Windows) -- it clones Wt and "
            "clones + builds Boost the same way it does SDL3 and VALO.")
    endif()

    # Boost is built statically by build.sh into ext/boost/stage/lib. Point Wt's
    # find_package(Boost) at exactly that and nothing else, so a system Boost
    # can never be picked up by accident.
    set(BOOST_ROOT            "${UILO_BOOST_DIR}" CACHE PATH "" FORCE)
    set(Boost_USE_STATIC_LIBS ON  CACHE BOOL "" FORCE)
    set(Boost_NO_BOOST_CMAKE  ON  CACHE BOOL "" FORCE)
    set(Boost_NO_SYSTEM_PATHS ON  CACHE BOOL "" FORCE)

    # Wt has a lot of optional pieces (PDF, image rendering, every Dbo database
    # backend, Qt interop, SSL, its own examples and tests). None of them are
    # reachable through the UILO API, so they stay off and the build stays small.
    # CONNECTOR_HTTP is the exception: it builds libwthttp, the stand-alone
    # server that uilo::wt::run() starts.
    #
    # SHARED_LIBS OFF forces static libs on every platform (Wt defaults to
    # shared on Linux/macOS), so the result is one self-contained executable
    # with no Wt libraries to locate at runtime.
    set(SHARED_LIBS        OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS        OFF CACHE BOOL "" FORCE)
    set(ENABLE_LIBWTTEST   OFF CACHE BOOL "" FORCE)
    set(ENABLE_LIBWTDBO    OFF CACHE BOOL "" FORCE)
    set(ENABLE_SQLITE      OFF CACHE BOOL "" FORCE)
    set(ENABLE_POSTGRES    OFF CACHE BOOL "" FORCE)
    set(ENABLE_FIREBIRD    OFF CACHE BOOL "" FORCE)
    set(ENABLE_MYSQL       OFF CACHE BOOL "" FORCE)
    set(ENABLE_MSSQLSERVER OFF CACHE BOOL "" FORCE)
    set(ENABLE_SSL         OFF CACHE BOOL "" FORCE)
    set(ENABLE_HARU        OFF CACHE BOOL "" FORCE)
    set(ENABLE_PANGO       OFF CACHE BOOL "" FORCE)
    set(ENABLE_QT4         OFF CACHE BOOL "" FORCE)
    set(ENABLE_QT5         OFF CACHE BOOL "" FORCE)
    set(ENABLE_QT6         OFF CACHE BOOL "" FORCE)
    set(CONNECTOR_HTTP     ON  CACHE BOOL "" FORCE)
    set(CONNECTOR_FCGI     OFF CACHE BOOL "" FORCE)

    # Build Wt itself as C++17. UILO needs C++20, but that only has to apply to
    # UILO's own targets, and holding Wt at the standard it is tested against
    # avoids inheriting our requirement onto 500 files of someone else's code.
    set(_uilo_saved_cxx_standard "${CMAKE_CXX_STANDARD}")
    set(CMAKE_CXX_STANDARD 17)
    add_subdirectory("${UILO_WT_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/wt")
    set(CMAKE_CXX_STANDARD "${_uilo_saved_cxx_standard}")

    # Wt's headers live in src/; WConfig.h is generated into its binary dir.
    set(UILO_WT_INCLUDE_DIRS
        "${UILO_WT_DIR}/src"
        "${CMAKE_CURRENT_BINARY_DIR}/wt")
endif()

if(NOT UILO_WT)

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

# Use a persistent build directory (.sdl3build in project root) that survives
# clean rebuilds. This way SDL3 only builds once.
set(UILO_SDL3_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.sdl3build")

# Only configure/build SDL3 if the library doesn't exist
# (subsequent runs skip this to avoid unnecessary rebuilds)
find_library(UILO_SDL3_BUILT NAMES SDL3 PATHS "${UILO_SDL3_BUILD_DIR}" NO_DEFAULT_PATH)
if(NOT UILO_SDL3_BUILT)
    add_subdirectory("${UILO_SDL3_DIR}" "${UILO_SDL3_BUILD_DIR}")
else()
    # SDL3 already built, create a target to link against
    add_library(SDL3-static STATIC IMPORTED GLOBAL)
    set_target_properties(SDL3-static PROPERTIES
        IMPORTED_LOCATION "${UILO_SDL3_BUILT}"
    )
    add_library(SDL3::SDL3-static ALIAS SDL3-static)
    target_include_directories(SDL3-static INTERFACE "${UILO_SDL3_DIR}/include")

    # Link against required system frameworks (same as when built via add_subdirectory)
    if(APPLE)
        target_link_libraries(SDL3-static INTERFACE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreFoundation"
            "-framework CoreGraphics"
            "-framework CoreVideo"
            "-framework CoreMedia"
            "-framework CoreAudio"
            "-framework AudioToolbox"
            "-framework GameController"
            "-framework AVFoundation"
            "-framework MetalKit"
            "-framework Metal"
            "-framework Carbon"
            "-framework CoreHaptics"
            "-framework ForceFeedback"
            "-framework UniformTypeIdentifiers"
        )
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(SDL3-static INTERFACE
            Threads::Threads
            ${CMAKE_DL_LIBS}
        )
    endif()
endif()

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
add_subdirectory("${UILO_VALO_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/valo")

endif() # NOT UILO_WT

# ---------------------------------------------------------------------------
# UILO library
# ---------------------------------------------------------------------------
file(GLOB_RECURSE UILO_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp"
)
# include/wt/ holds the headless backend; it replaces the renderer rather than
# adding to it, so it is only ever in the source list for a UILO_WT build.
list(FILTER UILO_SRCS EXCLUDE REGEX "/include/wt/")

if(UILO_WT)
    # Drop everything that talks to the GPU or the OS and substitute no-ops.
    # platform/ goes too: MacStubs.cpp is guarded on !__APPLE__, so the
    # headless backend supplies those symbols on every platform instead.
    list(FILTER UILO_SRCS EXCLUDE REGEX "/include/(renderer|platform)/")
    list(APPEND UILO_SRCS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/wt/HeadlessBackend.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/wt/Translator.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/wt/UiloWt.cpp")
elseif(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
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

if(UILO_WT)
    # Both backends drop their library into lib/, so they must not share a file
    # name: a desktop build would otherwise overwrite the web one (and vice
    # versa) and a consumer would silently link the wrong archive -- the build
    # directories are separate, but the output path is not.
    #   libuilo.a     desktop (VALO)
    #   libuilo_wt.a  web     (Wt)
    set_target_properties(uilo PROPERTIES OUTPUT_NAME uilo_wt)

    # UILO's input API names VALO's key, button and modifier types, which are
    # declared header-only in render/Window.hpp. The web build needs those
    # declarations and nothing else of VALO; the few Window members UILO calls
    # are stubbed in wt/HeadlessBackend.cpp, so VALO itself is never built.
    target_include_directories(uilo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ext/VALO/include")

    # Icon.cpp lives under include/elements/, so it is part of this build too and
    # still needs NanoSVG's headers to compile. Rasterizing is inert here (the
    # headless backend hands back an invalid texture and Icon bails), but the
    # element's markup accessor is exactly what a web translator consumes.
    target_include_directories(uilo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanosvg")

    # PUBLIC: it gates the web API at the bottom of UILO.hpp, so consumer TUs
    # need it defined too or including <UILO.hpp> gets them nothing.
    target_compile_definitions(uilo PUBLIC UILO_WT)

    # PUBLIC include dirs and libs so an application only has to link `uilo` --
    # Wt comes with it, and stays available for anyone who wants to reach past
    # the bridge via Session::application().
    target_include_directories(uilo PUBLIC ${UILO_WT_INCLUDE_DIRS})
    target_link_libraries(uilo PUBLIC wt wthttp Threads::Threads)

    # UILO's headers need C++20 (Palette's heterogeneous map lookups, among
    # others). PUBLIC so consumers don't have to know that.
    target_compile_features(uilo PUBLIC cxx_std_20)

    if(MSVC)
        target_compile_options(uilo PUBLIC /utf-8)
        target_compile_options(uilo PRIVATE /W3)
    else()
        target_compile_options(uilo PRIVATE -Wall -Wextra)
    endif()

    return()
endif()

target_include_directories(uilo PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/ext/stb"
    "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanosvg"
)

# VALO brings SDL3 with it (PUBLIC on its side), so an application only has to
# link `uilo`.
target_link_libraries(uilo PUBLIC VALO::VALO)

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
