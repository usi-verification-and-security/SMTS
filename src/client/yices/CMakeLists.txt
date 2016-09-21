find_package(YICES REQUIRED)

include_directories(${YICES_INCLUDE_DIR})
link_directories(${YICES_LIBRARY_DIR})

add_executable(solver_yices ../main.cpp SolverProcess.cpp)

target_link_libraries(solver_yices client yices)