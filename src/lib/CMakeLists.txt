find_package(Threads REQUIRED)

find_package(SQLITE3 REQUIRED)
include_directories(${SQLITE3_INCLUDE_DIR})
link_directories(${SQLITE3_LIBRARY_DIR})

add_library(Lib
        lib.cpp
        sqlite3/Connection.cpp
        sqlite3/Statement.cpp
        Thread.cpp
        Process.cpp
        Logger.h
        net/Address.cpp
        net/Header.cpp
        net/Lemma.cpp
        net/Pipe.cpp
        net/Server.cpp
        net/Socket.cpp
        )

target_link_libraries(Lib sqlite3 ${CMAKE_THREAD_LIBS_INIT})