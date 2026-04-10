find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_target(run_python_script
        COMMAND ${CMAKE_COMMAND} -E echo "Generating Shaders..."
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/shaders/gen_shaders.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating Shaders"
        VERBATIM
)
add_dependencies(Engine run_python_script)