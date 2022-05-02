/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#include <PTPLib/common/Exception.hpp>

#include <iostream>
#include "lib/Logger.h"
#include "Settings.h"
#include "LemmaServer.h"

int main(int argc, char **argv) {
    Settings settings = Settings();
    try {
        settings.load(argc, argv);
    }
    catch (PTPLib::common::Exception &ex) {
        Logger::log(Logger::ERROR, ex.what());
    }

    try {
        LemmaServer server(settings.port, settings.server, settings.db_filename, settings.send_again);
        server.run_forever();
    } catch (PTPLib::common::Exception &ex) {
        Logger::log(Logger::ERROR, ex.what());
    }
}