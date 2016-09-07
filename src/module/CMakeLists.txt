list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)

function(find NAME INCLUDE_NAMES LIBRARY_NAMES)
    if (${NAME}_FOUND)
        return()
    endif ()
    if (NOT ${NAME}_INCLUDE_DIR)
        find_path(${NAME}_INCLUDE_DIR NAMES ${INCLUDE_NAMES}
                PATHS ${${NAME}_PATH}/include)
    endif ()

    if (NOT ${NAME}_LIBRARY)
        find_library(${NAME}_LIBRARY NAMES ${LIBRARY_NAMES}
                PATHS ${${NAME}_PATH}/lib)
    endif ()

    if (${NAME}_INCLUDE_DIR AND ${NAME}_LIBRARY)
        get_filename_component(${NAME}_LIBRARY_DIR ${${NAME}_LIBRARY} PATH)
        set(${NAME}_FOUND TRUE)
    endif ()

    if (${NAME}_FOUND)
        message(STATUS "Using ${NAME}_INCLUDE_DIR: ${${NAME}_INCLUDE_DIR}")
        message(STATUS "Using ${NAME}_LIBRARY: ${${NAME}_LIBRARY}")
        set(${NAME}_FOUND TRUE PARENT_SCOPE)
    elseif (${NAME}_FIND_REQUIRED)
        message(FATAL_ERROR "\nCould not find ${NAME}:\n"
                "Hint path using -D${NAME}_PATH=\n"
                "Or -D${NAME}_INCLUDE_DIR= and -D${NAME}_LIBRARY=\n")
    endif ()
endfunction()