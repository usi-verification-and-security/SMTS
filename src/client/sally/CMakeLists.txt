find_package(SALLY REQUIRED)

include_directories("${SALLY_INCLUDE_DIR}")

add_executable(solver_sally ../main.cpp SolverProcess.cpp)

target_link_libraries(solver_sally client ${SALLY_LIBRARY})
