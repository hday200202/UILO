file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(src ${EXAMPLE_SOURCES})
    get_filename_component(tgt "${src}" NAME_WE)
    add_executable("${tgt}" "${src}")
    target_link_libraries("${tgt}" PRIVATE uilo)
    set_target_properties("${tgt}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    )
endforeach()
