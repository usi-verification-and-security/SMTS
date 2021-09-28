include(FetchContent)

FetchContent_Declare(
        opensmt
        GIT_REPOSITORY https://github.com/MasoudAsadzade/OpenSMT2.git
        GIT_TAG parallel-infra2
)
FetchContent_GetProperties(opensmt)

if(NOT opensmt_POPULATED)
    FetchContent_Populate(opensmt)
    add_subdirectory(${opensmt_SOURCE_DIR} ${opensmt_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
FetchContent_MakeAvailable(opensmt)

include_directories(${GMP_INCLUDE_DIR})
include_directories(${opensmt_SOURCE_DIR})

include_directories(${opensmt_SOURCE_DIR}/src/bin)
include_directories(${opensmt_SOURCE_DIR}/src/models)
include_directories(${opensmt_SOURCE_DIR}/src/api)
include_directories(${opensmt_SOURCE_DIR}/src/cnfizers)
include_directories(${opensmt_SOURCE_DIR}/src/common)
include_directories(${opensmt_SOURCE_DIR}/src/minisat/mtl)
include_directories(${opensmt_SOURCE_DIR}/src/minisat/core)
include_directories(${opensmt_SOURCE_DIR}/src/options)
include_directories(${opensmt_SOURCE_DIR}/src/parsers/smt2new)
include_directories(${opensmt_SOURCE_DIR}/src/proof)
include_directories(${opensmt_SOURCE_DIR}/src/simplifiers)
include_directories(${opensmt_SOURCE_DIR}/src/smtsolvers)
include_directories(${opensmt_SOURCE_DIR}/src/symmetry)
include_directories(${opensmt_SOURCE_DIR}/src/tsolvers)
include_directories(${opensmt_SOURCE_DIR}/src/tsolvers/axdiffsolver)
include_directories(${opensmt_SOURCE_DIR}/src/tsolvers/bvsolver)
include_directories(${opensmt_SOURCE_DIR}/src/tsolvers/lasolver)
include_directories(${opensmt_SOURCE_DIR}/src/tsolvers/dlsolver)
include_directories(${opensmt_SOURCE_DIR}/src/tsolvers/egraph)
include_directories(${opensmt_SOURCE_DIR}/src/tsolvers/emptysolver)
include_directories(${opensmt_SOURCE_DIR}/src/tsolvers/liasolver)
include_directories(${opensmt_SOURCE_DIR}/src/tsolvers/lrasolver)
include_directories(${opensmt_SOURCE_DIR}/src/pterms)
include_directories(${opensmt_SOURCE_DIR}/src/logics)
include_directories(${opensmt_SOURCE_DIR}/src/symbols)
include_directories(${opensmt_SOURCE_DIR}/src/sorts)

add_executable(solver_opensmt ../main.cpp SolverProcess.cpp)

set(solver_opensmt_flags "-w")

set_target_properties(solver_opensmt PROPERTIES
        COMPILE_FLAGS ${solver_opensmt_flags}
        )
target_link_libraries(solver_opensmt client OpenSMT-static ${GMP_LIBRARY} ${GMPXX_LIBRARY} z)
