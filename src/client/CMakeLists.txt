set(solver_type ${solver})
if(NOT solver_type)
    set(PARALLEL ON CACHE INTERNAL "")
    set(solver_type OpenSMT2)
    message(STATUS "Solver Type is set to OpenSMT2 by default.")
endif()

include(FetchContent)
SET(OPENSMT_HOME CACHE STRING "OpenSMT installation directory")
find_package(OpenSMT CONFIG PATHS ${OPENSMT_HOME} NO_DEFAULT_PATH)

if (OpenSMT_FOUND)
else(OpenSMT_FOUND)
    set(OpenSMT_repository "https://github.com/MasoudAsadzade/opensmt-verification.git")
    set(OpenSMT_branch communicationChannel)

    FetchContent_Declare(
            OpenSMT
            GIT_REPOSITORY ${OpenSMT_repository}
            GIT_TAG ${OpenSMT_branch}
            GIT_SHALLOW true
            GIT_PROGRESS true
    )

    set(BUILD_EXECUTABLES OFF CACHE INTERNAL "")
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    set(BUILD_STATIC_LIBS ON CACHE INTERNAL "")
    set(PACKAGE_TESTS OFF CACHE INTERNAL "")

    message(STATUS "SMTS's Client is fetching OpenSMT from ${OpenSMT_repository}")
    message(STATUS "OpenSMT Branch : ${OpenSMT_branch}")
    FetchContent_MakeAvailable(OpenSMT)

    add_library(OpenSMT::OpenSMT ALIAS OpenSMT-splitter-static)
endif(OpenSMT_FOUND)


add_library(client STATIC OBJECT "")

target_link_libraries(client PUBLIC Lib OpenSMT::OpenSMT)

target_sources(client
        PRIVATE Settings.cpp
        PRIVATE SolverServer.cpp
        PRIVATE Schedular.cc
        )

if (NOT OpenSMT_FOUND)
    target_compile_definitions(client PUBLIC OPENSMT_LOCAL_BUILD)
    message(STATUS "OpenSMT2 Directories:${opensmt_SOURCE_DIR}; ${opensmt_BINARY_DIR}")
    target_include_directories(client PUBLIC
           "${opensmt_SOURCE_DIR}/src/bin"
           "${opensmt_SOURCE_DIR}/src/symbols"
           "${opensmt_SOURCE_DIR}/src/minisat/mtl"
            ${opensmt_SOURCE_DIR}/src/minisat/core
            ${opensmt_SOURCE_DIR}/src/common
            ${opensmt_SOURCE_DIR}/src/sorts
            ${opensmt_SOURCE_DIR}/src/symbols
            ${opensmt_SOURCE_DIR}/src/options
            ${opensmt_SOURCE_DIR}/src/api
            ${opensmt_SOURCE_DIR}/src/tsolvers
            ${opensmt_SOURCE_DIR}/src/tsolvers/egraph
            ${opensmt_SOURCE_DIR}/src/tsolvers/lasolver
            ${opensmt_SOURCE_DIR}/src/tsolvers/lrasolver
            ${opensmt_SOURCE_DIR}/src/tsolvers/liasolver
            ${opensmt_SOURCE_DIR}/src/cnfizers
            ${opensmt_SOURCE_DIR}/src/pterms
            ${opensmt_SOURCE_DIR}/src/logics
            ${opensmt_SOURCE_DIR}/src/smtsolvers
            ${opensmt_SOURCE_DIR}/src/parsers/smt2new
            ${opensmt_SOURCE_DIR}/src/simplifiers
            ${opensmt_SOURCE_DIR}/src/rewriters
            ${opensmt_SOURCE_DIR}/src/proof
            ${opensmt_SOURCE_DIR}/src/models
            ${opensmt_SOURCE_DIR}/src/itehandler
            ${opensmt_SOURCE_DIR}/src/interpolation
            )
    message(STATUS "GMP dirs:${GMP_INCLUDE_DIR}")

endif()

add_subdirectory(${solver_type})

target_link_libraries(client PUBLIC PTPLib::PTPLib)

target_compile_definitions(client PRIVATE -DSMTS_VERSION=${SMTS_VERSION})

if (VERBOSE_THREAD)
    target_compile_definitions(client PUBLIC -DVERBOSE_THREAD)
endif()