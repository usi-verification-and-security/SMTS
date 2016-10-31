find_package(Z3)
if (NOT Z3_FOUND)
    include(ExternalProject)
    ExternalProject_Add(Z3
            GIT_REPOSITORY https://github.com/Z3Prover/z3
            UPDATE_COMMAND git pull
            PATCH_COMMAND python contrib/cmake/bootstrap.py create
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
            )
    set(Z3_INCLUDE_DIR ${CMAKE_BINARY_DIR}/install/include)
    set(Z3_LIBRARY_DIR ${CMAKE_BINARY_DIR}/install/lib)
endif ()

include_directories("${Z3_INCLUDE_DIR}")
link_directories("${Z3_LIBRARY_DIR}")

add_executable(solver_z3 ../main.cpp SolverProcess.cpp)

if (NOT Z3_FOUND)
    add_dependencies(solver_z3 Z3)
endif ()

target_link_libraries(solver_z3 client z3)