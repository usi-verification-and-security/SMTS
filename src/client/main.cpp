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

    FileThread *ft = nullptr;
    if (!settings.server.size() && settings.files.size()) {
        ft = new FileThread(settings);
    }

    if (settings.server.size()) {
        try {
            SolverServer ss(net::Address(settings.server));
            ss.run_forever();
        } catch (net::SocketException &ex) {
            Logger::log(Logger::ERROR, ex.what());
        }
    }
    delete ft;
    Logger::log(Logger::INFO, "all done. bye!");
}