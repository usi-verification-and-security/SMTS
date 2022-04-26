find_package(Threads REQUIRED)
if (SQLITE3_Enabled)
    find_package(SQLITE3)
    include_directories(${SQLITE3_INCLUDE_DIR})
endif()
add_library(Lib STATIC
        Thread.cpp
        Process.cpp
        Logger.h
        net/Address.cpp
        net/Server.cpp
        net/Socket.cpp
        )

target_compile_definitions(Lib PRIVATE -DSMTS_VERSION=${SMTS_VERSION})
if(DEBUG_SMTS)
    target_compile_definitions(Lib PUBLIC -DENABLE_DEBUGING)
endif()
if (SQLITE3_Enabled)
    target_link_libraries(Lib ${SQLITE3_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
    target_compile_definitions(Lib PUBLIC -DSQLITE_IS_ON)
endif()