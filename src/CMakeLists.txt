cmake_minimum_required (VERSION 2.6)
set (CMAKE_CXX_COMPILER  /usr/bin/g++)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Wall -Werror")
set (CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/../)
message ("bin_dir ${CMAKE_BINARY_DIR}")
message ("prject_bin_dir ${PROJECT_BINARY_DIR}")

enable_testing()

set (LIBRARY_OUTPUT_PATH
        ${CMAKE_BINARY_DIR}/lib
        CACHE PATH
        "Single directory for all libraries"
        )

set (EXECUTABLE_OUTPUT_PATH
        ${CMAKE_BINARY_DIR}/bin
        CACHE PATH
        "Single directory for all binaries"
        )
project(gexecutor)
add_subdirectory(gexecutor)
add_subdirectory(samples)


