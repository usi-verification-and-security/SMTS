//
// Created by Matteo Marescotti.
//

#include <iostream>
#include "lib/Logger.h"
#include "Settings.h"
#include "SolverServer.h"
#include "FileThread.h"


int main(int argc, char **argv) {
    Settings settings;
    try {
        settings.load(argc, argv);
    }
    catch (Exception ex) {
        std::cerr << "argument parsing error: " << ex.what() << "\n";
        exit(1);
    }

    std::unique_ptr<FileThread> ft;
    if (!settings.server.size() && settings.files.size()) {
        ft.reset(new FileThread(settings));
    }

    if (settings.server.size()) {
        try {
            SolverServer(net::Address(settings.server)).run_forever();
        } catch (net::SocketException &ex) {
            Logger::log(Logger::ERROR, ex.what());
            return -1;
        }
        Logger::log(Logger::INFO, "all done. bye!");
    }
}