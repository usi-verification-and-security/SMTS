/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#include "FileThread.h"
#include "lib/Logger.h"
#include "lib/Process.h"
#include "lib/Thread.h"
#include "lib/sqlite3.h"
#include "lib/net.h"

#include <PTPLib/common/Memory.hpp>
#include <PTPLib/common/Lib.hpp>

#include <iostream>
#include <fstream>

FileThread::FileThread(Settings &settings) :
        settings(settings),
        server((uint16_t) 0) {
    if (settings.server.size())
        throw PTPLib::common::Exception(__FILE__, __LINE__, "server must be null");
    settings.server = to_string(this->server.get_local());
    this->main();
}

void FileThread::main() {
    PTPLib::net::Header header;

    std::shared_ptr<net::Socket> client(this->server.accept());
    std::shared_ptr<net::Socket> lemmas;

    if (this->settings.lemmas.size()) {
        try {
            lemmas.reset(new net::Socket(this->settings.lemmas));
        } catch (net::SocketException) {
        }
        header[PTPLib::common::Param.COMMAND] = PTPLib::common::Command.LEMMAS;
        header[PTPLib::common::Command.LEMMAS] = this->settings.lemmas;

        client->write(PTPLib::net::SMTS_Event(header));
    }

    for (auto &filename : this->settings.files) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            Logger::log(Logger::WARNING, "unable to open: " + filename);
            continue;
        }

        std::string payload;
        file.seekg(0, std::ios::end);
        payload.resize((unsigned long) file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(&payload[0], payload.size());
        file.close();

        header.clear();
        for (auto &it : this->settings.parameters) {
            header.set(PTPLib::net::parameter, it.first, it.second);
        }
        header[PTPLib::common::Param.COMMAND] = PTPLib::common::Command.SOLVE;
        header[PTPLib::common::Param.NAME] = filename;
        header[PTPLib::common::Param.NODE] = "[]";
        client->write(PTPLib::net::SMTS_Event(std::move(header), std::move(payload)));
        if (this->settings.dump_clauses) {
            header[PTPLib::common::Param.COMMAND] = PTPLib::common::Command.CNFCLAUSES;
            client->write(PTPLib::net::SMTS_Event(std::move(header), std::move(payload)));
        }
        do {
            uint32_t length = 0;
            auto SMTS_event = client->read(length);
            if (SMTS_event.header[PTPLib::common::Param.REPORT] == PTPLib::common::Command.CNFCLAUSES) {
                auto clauses_filename = filename + ".cnf";
                std::ofstream clauses_file(clauses_filename);
                clauses_file << SMTS_event.body << "\n";
                Logger::log(Logger::INFO, "clauses stored in " + clauses_filename);
                break;
            }
            if (this->settings.verbose) {
                Logger::log(Logger::INFO, header);
            }
            if (header[PTPLib::common::Param.REPORT] == "sat" || header[PTPLib::common::Param.REPORT] == "unsat" || header[PTPLib::common::Param.REPORT] == "unknown") {
                if (!this->settings.dump_clauses)
                    break;
            }
        } while (true);
        if (lemmas and !this->settings.keep_lemmas) {
            try {
                header[PTPLib::common::Command.LEMMAS] = "0";
                lemmas->write(PTPLib::net::SMTS_Event(std::move(header)));
            } catch (net::SocketException) {
                lemmas.reset();
            }
        }
        header[PTPLib::common::Param.COMMAND] = PTPLib::common::Command.STOP;
        header[PTPLib::common::Param.NAME] = filename;
        header[PTPLib::common::Param.NODE] = "[]";
        client->write(PTPLib::net::SMTS_Event(std::move(header)));
    }
}