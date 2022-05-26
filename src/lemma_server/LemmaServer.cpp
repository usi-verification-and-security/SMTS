/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#include "LemmaServer.h"
#include "lib/Logger.h"

#include <PTPLib/common/Lib.hpp>
#include <PTPLib/net/Header.hpp>

#include <string>
#include <algorithm>

LemmaServer::LemmaServer(uint16_t port, const std::string &server, const std::string &db_filename, bool send_again) :
        Server(port),
        send_again(send_again) {
    if (server.size()) {
        this->server.reset(new net::Socket(server));
        PTPLib::net::Header header;
        header[PTPLib::common::Command.LEMMAS] = ":" + std::to_string(port);
        this->server->write(PTPLib::net::SMTS_Event(std::move(header), std::string()));
        this->add_socket(this->server);
    }

#ifdef SQLITE_IS_ON
    if (db_filename.size()) {
        this->db.reset(new SQLite3::Connection(db_filename));
        this->db->exec("CREATE TABLE IF NOT EXISTS Push("
                               "id INTEGER PRIMARY KEY, "
                               "ts INTEGER, "
                               "name TEXT, "
                               "node TEXT, "
                               "data TEXT"
                               ");");
        this->db->exec("CREATE TABLE IF NOT EXISTS Lemma("
                               "id INTEGER PRIMARY KEY, "
                               "pid INTEGER REFERENCES Push(id), "
                               "level INTEGER, "
                               "score INTEGER, "
                               "smtlib TEXT"
                               ");");
    }
#endif
};

void LemmaServer::handle_accept(net::Socket const & client) {
    Logger::log(Logger::INFO, "+ " + to_string(client.get_remote()));
}

void LemmaServer::handle_close(net::Socket & client) {
    Logger::log(Logger::INFO, "- " + to_string(client.get_remote()));
    if (&client == this->server.get())
    {
        Logger::log(Logger::INFO, "server connection closed.");
        this->stop();
        return;
    }
    for (auto const & pair : this->solvers) {
        if (this->solvers[pair.first].count(&client)) {
            this->solvers[pair.first].erase(&client);
        }
    }
}

void LemmaServer::handle_exception(net::Socket const & client, const std::exception & ex) {
    Logger::log(Logger::WARNING, "Exception from: " + to_string(client.get_remote()) + ": " + ex.what());
}


