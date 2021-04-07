add_executable(lemma_server main.cpp Settings.cpp LemmaServer.cpp)

target_link_libraries(lemma_server Lib)
target_compile_definitions(lemma_server PRIVATE -DSMTS_VERSION=${SMTS_VERSION})