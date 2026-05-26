cmake_minimum_required(VERSION 3.16)
project(UILO LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

file(GLOB_RECURSE UILO_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp"
)

add_library(uilo STATIC ${UILO_SRCS})

target_include_directories(uilo PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/elements"
)

target_link_libraries(uilo PUBLIC SFML::Graphics SFML::Window SFML::System)
target_compile_options(uilo PRIVATE -Wall -Wextra)

add_subdirectory(examples)
