add_executable(lemma_server main.cpp Settings.cpp LemmaServer.cpp Server.cpp)

target_link_libraries(lemma_server Lib PTPLib::PTPLib)

target_compile_definitions(lemma_server PRIVATE -DSMTS_VERSION=${SMTS_VERSION})

if (ACTIVELOG)
    target_compile_definitions(lemma_server PUBLIC -DSMTS_ACTIVELOG)
endif()

if (SQLITE3_Enabled)
    target_compile_definitions(lemma_server PUBLIC -DSQLITE_IS_ON)
endif()