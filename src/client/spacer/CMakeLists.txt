find_package(SPACER)
if (NOT SPACER_FOUND)
    include(ExternalProject)
    ExternalProject_Add(Spacer
            GIT_REPOSITORY https://bitbucket.org/maresm/spacer
            GIT_TAG spacer3
            UPDATE_COMMAND git pull
            PATCH_COMMAND python contrib/cmake/bootstrap.py create
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install -DENABLE_TRACING_FOR_NON_DEBUG=ON
            )
    set(SPACER_INCLUDE_DIR ${CMAKE_BINARY_DIR}/install/include)
    set(SPACER_LIBRARY_DIR ${CMAKE_BINARY_DIR}/install/lib)
endif ()

include_directories("${SPACER_INCLUDE_DIR}")
link_directories("${SPACER_LIBRARY_DIR}")

add_executable(solver_spacer ../main.cpp SolverProcess.cpp)

if (NOT SPACER_FOUND)
    add_dependencies(solver_spacer Spacer)
endif ()

target_link_libraries(solver_spacer client z3)

add_executable(test_spacer ./main.cpp)
target_link_libraries(test_spacer z3)