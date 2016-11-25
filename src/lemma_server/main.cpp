#include <iostream>
#include "lib/Logger.h"
#include "Settings.h"
#include "LemmaServer.h"


int main(int argc, char **argv) {
    Settings settings = Settings();
    try {
        settings.load(argc, argv);
    }
    catch (Exception &ex) {
        Logger::log(Logger::ERROR, ex.what());
    }

    try {
        LemmaServer server(settings.port, settings.server, settings.db_filename);
        server.run_forever();
    } catch (Exception &ex) {
        Logger::log(Logger::ERROR, ex.what());
    }
}