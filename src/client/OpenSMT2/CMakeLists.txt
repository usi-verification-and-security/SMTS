
add_executable(solver_opensmt ../main.cpp SolverProcess.cpp)

set(solver_opensmt_flags "-w")

set_target_properties(solver_opensmt PROPERTIES COMPILE_FLAGS ${solver_opensmt_flags})

target_link_libraries(solver_opensmt client ${GMP_LIBRARY} ${GMPXX_LIBRARY})
