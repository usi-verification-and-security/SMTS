list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)

function(find NAME INCLUDE_NAMES LIBRARY_NAMES)
    if (${NAME}_FOUND)
        return()
    endif ()

    if (${NAME}_DIR)
        get_filename_component(${NAME}_ABS_DIR ${${NAME}_DIR}
                REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
    endif ()

    if (NOT ${NAME}_INCLUDE_DIR)
        set(${NAME}_INCLUDE_DIR "${${NAME}_INCLUDE_DIR}-NOTFOUND")
        find_path(${NAME}_INCLUDE_DIR NAMES ${INCLUDE_NAMES}
                PATHS ${${NAME}_ABS_DIR}
                PATH_SUFFIXES "" "include" NO_DEFAULT_PATH)
        find_path(${NAME}_INCLUDE_DIR NAMES ${INCLUDE_NAMES})
    endif ()

    if (NOT ${NAME}_LIBRARY)
        set(${NAME}_LIBRARY "${${NAME}_LIBRARY}-NOTFOUND")
        find_library(${NAME}_LIBRARY NAMES ${LIBRARY_NAMES}
                PATHS ${${NAME}_ABS_DIR}
                PATH_SUFFIXES "" "lib" "lib64" NO_DEFAULT_PATH)
        find_library(${NAME}_LIBRARY NAMES ${LIBRARY_NAMES})
    endif ()

    if (${NAME}_INCLUDE_DIR AND ${NAME}_LIBRARY)
        get_filename_component(${NAME}_LIBRARY_DIR ${${NAME}_LIBRARY} PATH)
        set(${NAME}_FOUND TRUE)
    endif ()

    set(${NAME}_DIR "" CACHE PATH "Hint ${NAME} install path")

    if (${NAME}_FOUND)
        message(STATUS "Using ${NAME}_INCLUDE_DIR: ${${NAME}_INCLUDE_DIR}")
        message(STATUS "Using ${NAME}_LIBRARY_DIR: ${${NAME}_LIBRARY_DIR}")
        set(${NAME}_LIBRARY_DIR ${${NAME}_LIBRARY_DIR} PARENT_SCOPE)
        set(${NAME}_FOUND TRUE PARENT_SCOPE)
    else ()
        if (${NAME}_FIND_REQUIRED)
            set(TYPE FATAL_ERROR)
        else ()
            set(TYPE STATUS)
        endif ()
        message(${TYPE} "\nCould not find ${NAME}:\n"
                "Provide install path in ${NAME}_DIR\n")
    endif ()
endfunction()