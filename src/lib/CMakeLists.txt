find_package(Threads REQUIRED)
find_package(SQLITE3)
if (SQLITE3_FOUND)
    include_directories(${SQLITE3_INCLUDE_DIR})
endif()
add_library(Lib STATIC
        lib.cpp
        memory.cpp
        Thread.cpp
        Process.cpp
        Logger.h
        net/Address.cpp
        net/Header.cpp
        net/Pipe.cpp
        net/Server.cpp
        net/Socket.cpp
        sqlite3/Connection.cpp
        sqlite3/Statement.cpp
        )

target_compile_definitions(Lib PRIVATE -DSMTS_VERSION=${SMTS_VERSION})
if(DEBUG_SMTS)
    target_compile_definitions(Lib PUBLIC -DENABLE_DEBUGING)
endif()
if (SQLITE3_FOUND)
    target_link_libraries(Lib ${SQLITE3_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
    target_compile_definitions(Lib PUBLIC -DSQLITE_IS_ON)
endif()