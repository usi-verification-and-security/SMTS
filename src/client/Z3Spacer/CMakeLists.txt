find_package(Z3SPACER)
if (NOT Z3SPACER_FOUND)
    include(ExternalProject)
    ExternalProject_Add(Z3Spacer
            GIT_REPOSITORY https://bitbucket.org/maresm/spacer
            GIT_TAG spacer3
            UPDATE_COMMAND git pull
            PATCH_COMMAND python contrib/cmake/bootstrap.py create
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install -DENABLE_TRACING_FOR_NON_DEBUG=ON -DBUILD_PYTHON_BINDINGS=ON -DCMAKE_INSTALL_PYTHON_PKG_DIR=${CMAKE_BINARY_DIR}/install
            )
    set(Z3SPACER_INCLUDE_DIR ${CMAKE_BINARY_DIR}/install/include)
    set(Z3SPACER_LIBRARY_DIR ${CMAKE_BINARY_DIR}/install/lib)
endif ()

include_directories("${Z3SPACER_INCLUDE_DIR}")
link_directories("${Z3SPACER_LIBRARY_DIR}")

add_executable(solver_z3spacer ../main.cpp SolverProcess.cpp)

if (NOT Z3SPACER_FOUND)
    add_dependencies(solver_z3spacer Z3Spacer)
endif ()

target_link_libraries(solver_z3spacer client z3)

add_executable(test_spacer ./main.cpp)
target_link_libraries(test_spacer Lib)
