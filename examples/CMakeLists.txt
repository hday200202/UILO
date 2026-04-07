cmake_minimum_required(VERSION 3.20)
project(UILO_Examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Shared UILO sources
set(UILO_SRCS
    ../include/UILO.cpp
    ../include/Page.cpp
    ../include/elements/Element.cpp
    ../include/elements/Modifier.cpp
    ../include/elements/containers/Container.cpp
    ../include/elements/containers/Column.cpp
    ../include/elements/containers/Row.cpp
    ../include/elements/decor/Spacer.cpp
    ../include/elements/decor/Text.cpp
    ../include/elements/decor/Image.cpp
)

include_directories(../include)

# --- SDL3 basic ---
option(BUILD_SDL_BASIC "Build the SDL3 basic example" OFF)
if(BUILD_SDL_BASIC)
    find_package(SDL3 REQUIRED)
    find_package(SDL3_ttf REQUIRED)
    find_package(SDL3_image REQUIRED)

    add_executable(sdl_basic sdl_basic.cpp ${UILO_SRCS})
    target_link_libraries(sdl_basic PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf SDL3_image::SDL3_image)

    if(APPLE)
        find_library(OPENGL_FRAMEWORK OpenGL)
        target_link_libraries(sdl_basic PRIVATE ${OPENGL_FRAMEWORK})
    elseif(UNIX)
        find_package(OpenGL REQUIRED)
        target_link_libraries(sdl_basic PRIVATE OpenGL::GL)
    endif()
endif()

# --- SFML basic ---
option(BUILD_SFML_BASIC "Build the SFML basic example" OFF)
if(BUILD_SFML_BASIC)
    find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

    add_executable(sfml_basic sfml_basic.cpp ${UILO_SRCS})
    target_link_libraries(sfml_basic PRIVATE SFML::Graphics SFML::Window SFML::System)

    if(APPLE)
        find_library(OPENGL_FRAMEWORK OpenGL)
        target_link_libraries(sfml_basic PRIVATE ${OPENGL_FRAMEWORK})
    elseif(UNIX)
        find_package(OpenGL REQUIRED)
        target_link_libraries(sfml_basic PRIVATE OpenGL::GL)
    endif()
endif()

# --- Metal basic (macOS only) ---
if(APPLE)
    option(BUILD_METAL_BASIC "Build the Metal basic example (macOS only)" OFF)
    if(BUILD_METAL_BASIC)
        enable_language(OBJCXX)
        add_executable(metal_basic metal_basic.mm ${UILO_SRCS})
        target_compile_options(metal_basic PRIVATE -std=c++20)

        find_library(METAL_FRAMEWORK Metal)
        find_library(COCOA_FRAMEWORK Cocoa)
        find_library(QUARTZCORE_FRAMEWORK QuartzCore)
        target_link_libraries(metal_basic PRIVATE
            ${METAL_FRAMEWORK}
            ${COCOA_FRAMEWORK}
            ${QUARTZCORE_FRAMEWORK}
        )
    endif()
endif()
