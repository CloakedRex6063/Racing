function(copy_dirs TARGET_NAME)
    foreach(DIR ${ARGN})
        cmake_path(GET DIR STEM DIR_NAME)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${DIR}"
                "${CMAKE_BINARY_DIR}/game/${DIR_NAME}"
                COMMENT "Copying ${DIR}"
        )
    endforeach()
endfunction()