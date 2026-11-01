# Keep the downstream fix across build.sh dependency clones until VALO ships it.
find_package(Git REQUIRED)
set(_uilo_valo_patch "${CMAKE_CURRENT_LIST_DIR}/patches/valo-windowing.patch")
execute_process(
    COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${_uilo_valo_patch}"
    WORKING_DIRECTORY "${UILO_VALO_DIR}"
    RESULT_VARIABLE _uilo_patch_present OUTPUT_QUIET ERROR_QUIET)
if(NOT _uilo_patch_present EQUAL 0)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" apply --check "${_uilo_valo_patch}"
        WORKING_DIRECTORY "${UILO_VALO_DIR}"
        RESULT_VARIABLE _uilo_patch_check ERROR_VARIABLE _uilo_patch_error)
    if(NOT _uilo_patch_check EQUAL 0)
        message(FATAL_ERROR "[UILO] VALO windowing patch needs updating: ${_uilo_patch_error}")
    endif()
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" apply "${_uilo_valo_patch}"
        WORKING_DIRECTORY "${UILO_VALO_DIR}"
        RESULT_VARIABLE _uilo_patch_apply ERROR_VARIABLE _uilo_patch_error)
    if(NOT _uilo_patch_apply EQUAL 0)
        message(FATAL_ERROR "[UILO] Could not patch VALO: ${_uilo_patch_error}")
    endif()
endif()
