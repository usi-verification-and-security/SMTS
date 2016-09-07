add_executable(solver_opensmt ../main.cpp SolverProcess.cpp OpenSMTSolver.cpp)

find_package(GMP REQUIRED)
if (NOT GMP_PATH)
    get_filename_component(GMP_PATH ${GMP_INCLUDE_DIR} PATH)
endif ()

include(ExternalProject)
ExternalProject_Add(OPENSMT2
        GIT_REPOSITORY https://scm.ti-edu.ch/repogit/opensmt2.git
        UPDATE_COMMAND git pull
        PATCH_COMMAND autoreconf --force --install
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ./configure --with-gmp=${GMP_PATH}
        BUILD_COMMAND make
        INSTALL_COMMAND ""
        )

add_dependencies(solver_opensmt OPENSMT2)
ExternalProject_Get_Property(OPENSMT2 SOURCE_DIR)

include_directories(
        "${GMP_INCLUDE_DIR}"
        "${SOURCE_DIR}/src"
        "${SOURCE_DIR}/src/minisat/core"
        "${SOURCE_DIR}/src/minisat/mtl"
        "${SOURCE_DIR}/src/common"
        "${SOURCE_DIR}/src/logics"
        "${SOURCE_DIR}/src/options"
        "${SOURCE_DIR}/src/symbols"
        "${SOURCE_DIR}/src/api"
        "${SOURCE_DIR}/src/pterms"
        "${SOURCE_DIR}/src/sorts"
        "${SOURCE_DIR}/src/smtsolvers"
        "${SOURCE_DIR}/src/tsolvers"
        "${SOURCE_DIR}/src/cnfizers"
        "${SOURCE_DIR}/src/tsolvers/egraph"
        "${SOURCE_DIR}/src/tsolvers/lrasolver"
        "${SOURCE_DIR}/src/parsers/smt2new"
)

link_directories("${GMP_LIBRARY_DIR}")

set(solver_opensmt_flags "-DIGNORE_DL_THEORYPROPAGATION -DOPTIMIZE -DNDEBUG -DOSMT1_SUBSTITUTION -DCUSTOM_EL_ALLOC -DTERMS_HAVE_EXPLANATIONS -DUSE_GZ -w -lz -lgmpxx -lgmp -lreadline -ltermcap -L/usr/local/opt/gmp/lib/")

add_library(opensmt STATIC IMPORTED)
set_target_properties(opensmt PROPERTIES
        IMPORTED_LOCATION ${SOURCE_DIR}/src/.libs/libopensmt.a
        )

set_target_properties(solver_opensmt PROPERTIES
        COMPILE_FLAGS ${solver_opensmt_flags}
        )

target_link_libraries(solver_opensmt client opensmt ${solver_opensmt_flags})