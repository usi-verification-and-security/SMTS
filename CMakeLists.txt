cmake_minimum_required(VERSION 3.2)

project(SMTS C CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_SOURCE_DIR "src")

get_filename_component(CMAKE_MODULE_PATH
        "${CMAKE_SOURCE_DIR}/../cmake_modules/" ABSOLUTE)


if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

if (UNIX)
    SET(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "lib")
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES} ".so" ".a")
endif ()

execute_process(COMMAND ${CMAKE_CURRENT_LIST_DIR}/version.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE SMTS_VERSION)
add_definitions(-DSMTS_VERSION=${SMTS_VERSION})

include_directories("src")

add_subdirectory(src/module)

add_subdirectory(src/lib)
add_subdirectory(src/lemma_server)
add_subdirectory(src/client)

message(STATUS "CMAKE_BUILD_TYPE selected to ${CMAKE_BUILD_TYPE}")