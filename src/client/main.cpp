/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */


#include "lib/Logger.h"
#include "Settings.h"
#include "SolverServer.h"
#include "FileThread.h"

#include <PTPLib/common/Exception.hpp>

#include <iostream>

int main(int argc, char **argv) {
    Settings settings;
    try {
        settings.load(argc, argv);
    }
    catch (PTPLib::common::Exception ex) {
        std::cerr << "argument parsing error: " << ex.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    if (settings.server.size()) {
        try {
            SolverServer(net::Address(settings.server)).run_forever();
        } catch (net::SocketException &ex) {
            Logger::log(Logger::ERROR, ex.what());
            return -1;
        }
        catch (...) {
        }
        Logger::log(Logger::INFO, "all done. bye!");
    }
}