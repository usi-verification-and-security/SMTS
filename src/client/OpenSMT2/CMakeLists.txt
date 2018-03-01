find_package(OPENSMT2 REQUIRED)

#if (NOT OPENSMT2_FOUND)
#    include(ExternalProject)
#    ExternalProject_Add(OpenSMT2
#            GIT_REPOSITORY https://scm.ti-edu.ch/repogit/opensmt2.git
#            UPDATE_COMMAND git pull
#            PATCH_COMMAND autoreconf --force --install
#            BUILD_IN_SOURCE 1
#            CONFIGURE_COMMAND ./configure --enable-library --prefix=${CMAKE_BINARY_DIR}/install --with-gmp=${GMP_PATH}
#            BUILD_COMMAND make
#            INSTALL_COMMAND make install
#            )
#    set(OPENSMT2_INCLUDE_DIR ${CMAKE_BINARY_DIR}/install/include)
#    set(OPENSMT2_LIBRARY_DIR ${CMAKE_BINARY_DIR}/install/lib)
#endif ()

include_directories(${OPENSMT2_INCLUDE_DIR}/opensmt)

add_executable(solver_opensmt ../main.cpp SolverProcess.cpp OpenSMTSolver.cpp)

#if (NOT OPENSMT2_FOUND)
#    add_dependencies(solver_opensmt OpenSMT2)
#endif ()

set(solver_opensmt_flags "-DPRODUCE_PROOF -DIGNORE_DL_THEORYPROPAGATION -DOPTIMIZE -DNDEBUG -DOSMT1_SUBSTITUTION -DCUSTOM_EL_ALLOC -DTERMS_HAVE_EXPLANATIONS -DUSE_GZ -w")

set_target_properties(solver_opensmt PROPERTIES
        COMPILE_FLAGS ${solver_opensmt_flags}
        )

target_link_libraries(solver_opensmt client ${OPENSMT2_LIBRARY} gmp gmpxx z readline)