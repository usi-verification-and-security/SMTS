include(FetchContent)
SET(OPENSMT_HOME CACHE STRING "OpenSMT installation directory")
find_package(OpenSMT CONFIG PATHS ${OPENSMT_HOME} NO_DEFAULT_PATH)

if (OpenSMT_FOUND)
else(OpenSMT_FOUND)
    string(ASCII 27 ESCAPE)
    set(PINK [35m)
    set(TM [0m)

    set(solver_type ${solver})
    if(NOT solver_type)
        set(PARALLEL ON CACHE INTERNAL "")
        set(solver_type OpenSMT2)
        message(STATUS "Solver Type is set to OpenSMT2 by default.")
    endif()

    if(NOT osmt_repo)
        message(STATUS "${ESCAPE}${PINK} You can choose which repository of ${solver_type} to fetch by: ${ESCAPE}${TM} ${ESCAPE}${PINK}-Dosmt_repo=<repository>${ESCAPE}${TM}")
        set(osmt_repo "https://github.com/usi-verification-and-security/opensmt.git")
    endif()

    if(NOT osmt_branch)
        message(STATUS "${ESCAPE}${PINK} You can choose which branch of ${solver_type} to fetch by: ${ESCAPE}${TM} ${ESCAPE}${PINK}-Dosmt_branch=<branch>${ESCAPE}${TM}")
        set(osmt_branch v2.9.1)
    endif()

    FetchContent_Declare(
            OpenSMT
            GIT_REPOSITORY ${osmt_repo}
            GIT_TAG ${osmt_branch}
            GIT_SHALLOW true
            GIT_PROGRESS true
    )

    set(BUILD_EXECUTABLES OFF CACHE INTERNAL "")
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    set(BUILD_STATIC_LIBS ON CACHE INTERNAL "")
    set(PACKAGE_TESTS OFF CACHE INTERNAL "")

    set(GREEN [32m)
    set(BLUE [34m)
    message(STATUS "${ESCAPE}${GREEN} ${solver_type} Repository: ${ESCAPE}${TM} ${ESCAPE}${BLUE}${osmt_repo}${ESCAPE}${TM}")
    message(STATUS "${ESCAPE}${GREEN} ${solver_type} BRANCH: ${ESCAPE}${TM}${ESCAPE}${BLUE}${osmt_branch}${ESCAPE}${TM}")

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
        ${opensmt_SOURCE_DIR}/src
    )
    message(STATUS "GMP dirs:${GMP_INCLUDE_DIR}")

endif()

add_subdirectory(${solver_type})

target_link_libraries(client PUBLIC PTPLib::PTPLib)

target_compile_definitions(client PRIVATE -DSMTS_VERSION=${SMTS_VERSION})

if (VERBOSE_THREAD)
    target_compile_definitions(client PUBLIC -DVERBOSE_THREAD)
endif()
