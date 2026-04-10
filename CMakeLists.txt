cmake_minimum_required(VERSION 4.1)
project(Racing)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(scripts/CPM.cmake)
include(scripts/functions.cmake)

add_subdirectory(engine)
add_subdirectory(game)

include(scripts/shaders.cmake)