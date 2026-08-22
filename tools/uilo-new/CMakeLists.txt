# The project generator: one target, two front-ends. Built alongside UILO so a
# library change that breaks a template breaks the build here rather than in
# someone's new project.
add_executable(uilo-new
    main.cpp
    gui.cpp
    scaffold.cpp
)

target_link_libraries(uilo-new PRIVATE uilo)

set_target_properties(uilo-new PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY         "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin")
