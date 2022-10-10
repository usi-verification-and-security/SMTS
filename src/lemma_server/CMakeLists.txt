add_executable(lemma_server main.cpp Settings.cpp LemmaServer.cpp ../lib/net/Server.cpp)

target_link_libraries(lemma_server Lib PTPLib::PTPLib)

target_compile_definitions(lemma_server PRIVATE -DSMTS_VERSION=${SMTS_VERSION})
