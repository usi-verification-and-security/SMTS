cmake_minimum_required(VERSION 3.2)

project(clause_server C CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

if (UNIX)
    SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")
endif ()

include_directories("src")

add_subdirectory(src/module)

add_subdirectory(src/lib)
add_subdirectory(src/lemma_server)
add_subdirectory(src/client)

message(STATUS "CMAKE_BUILD_TYPE selected to ${CMAKE_BUILD_TYPE}")