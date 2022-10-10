find_package(Threads REQUIRED)
if (SQLITE3_Enabled)
    find_package(SQLITE3)
    include_directories(${SQLITE3_INCLUDE_DIR})
endif()
add_library(Lib STATIC
        Logger.h
        net/Address.cpp
        net/Server.cpp
        net/Socket.cpp
        net/Report.h
        )

target_compile_definitions(Lib PRIVATE -DSMTS_VERSION=${SMTS_VERSION})

target_link_libraries(Lib PUBLIC PTPLib::PTPLib Threads::Threads)