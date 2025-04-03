cmake_minimum_required(VERSION 3.5)

if (POLICY CMP0074) # Policy 0074 has been introduced in CMake 3.12, so we need a check, otherwise older version would give an error
    cmake_policy(SET CMP0074 NEW)
endif()

execute_process(COMMAND ${CMAKE_CURRENT_LIST_DIR}/version.sh WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE SMTS_VERSION)
project(smts VERSION ${SMTS_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_SOURCE_DIR "src")

get_filename_component(CMAKE_MODULE_PATH
        "${CMAKE_SOURCE_DIR}/../cmake_modules/" ABSOLUTE)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()


if (UNIX)
    SET(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "lib")
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES} ".so" ".a")
endif ()

if (VERBOSE_THREAD)
    set(VERBOSE_THREAD ON CACHE INTERNAL "")
endif(VERBOSE_THREAD)

include_directories(${CMAKE_SOURCE_DIR})
add_subdirectory(${CMAKE_SOURCE_DIR}/module)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib)
add_subdirectory(${CMAKE_SOURCE_DIR}/lemma_server)
add_subdirectory(${CMAKE_SOURCE_DIR}/client)

message(STATUS "CMAKE_BUILD_TYPE set to ${CMAKE_BUILD_TYPE}")
message(STATUS "Version: ${SMTS_VERSION}")
