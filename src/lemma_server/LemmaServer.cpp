/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#include "LemmaServer.h"
#include "lib/Logger.h"
#include "lib/net/Report.h"

#include <PTPLib/common/Lib.hpp>
#include <PTPLib/net/Header.hpp>
#include <PTPLib/common/Memory.hpp>

#include <string>
#include <algorithm>
#include <thread>

LemmaServer::LemmaServer(uint16_t port, const std::string &server, const std::string &db_filename, bool send_again) :
        Server(port),
        send_again(send_again) {
    if (server.size()) {
        this->server.reset(new net::Socket(server));
        PTPLib::net::Header header;
        header[PTPLib::common::Command.LEMMAS] = ":" + std::to_string(port);
        this->server->write(PTPLib::net::SMTS_Event(std::move(header)));
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

void LemmaServer::notify_reset() {
    std::scoped_lock<std::mutex> s_lk(getChannel().getMutex());
    channel.setReset();
    channel.notify_one();
}

void LemmaServer::memory_checker(int max_memory)
{
    size_t limit = static_cast<std::size_t>(max_memory);
    if (limit == 0)
        return;

    while (true) {
        size_t memory_size_b = PTPLib::common::current_memory();
        if (memory_size_b > limit *1024 * 1024 ) {
            std::scoped_lock<std::mutex> lk(getChannel().getMutex());
            net::Report::error(getSMTS_serverSocket()," max memory reached from lemma-server: " + std::to_string(memory_size_b));
        }
        std::unique_lock<std::mutex> lk(getChannel().getMutex());
        if (getChannel().wait_for_reset(lk, std::chrono::seconds (3)))
            break;

        if (not lk.owns_lock()) {
            net::Report::error(getSMTS_serverSocket(), std::string(__FUNCTION__) + " can't take the lock");
        }
    }
}

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

void LemmaServer::handle_event(net::Socket & client, PTPLib::net::SMTS_Event && SMTS_Event)  {
    if (SMTS_Event.header.count("enableLog") == 1 and SMTS_Event.header.at("enableLog") == "1") {
        logEnabled = true;
    }
    if (SMTS_Event.header.count(PTPLib::common::Param.MAX_MEMORY) == 1) {
        std::thread t_mem_check([&] {
            memory_checker(atoi(SMTS_Event.header.at(PTPLib::common::Param.MAX_MEMORY).c_str()));
        });
        t_mem_check.detach();
        return;
    }
    if (SMTS_Event.header.count(PTPLib::common::Param.NAME) == 0 or
        SMTS_Event.header.count(PTPLib::common::Param.NODE) == 0 or
        SMTS_Event.header.count(PTPLib::common::Command.LEMMAS) == 0) {
        net::Report::error(getSMTS_serverSocket(),SMTS_Event.header, " invalid solver event from " + to_string(client.get_remote()));
        return;
    }
    if (SMTS_Event.header[PTPLib::common::Param.NODE].size() < 2) {
        net::Report::error(getSMTS_serverSocket(),SMTS_Event.header, " invalid solver branch from " + to_string(client.get_remote()));
        return;
    }
    if (this->lemmas.count(SMTS_Event.header[PTPLib::common::Param.NAME]) != 1)
        this->lemmas[SMTS_Event.header[PTPLib::common::Param.NAME]];

    uint32_t clauses_request = 0;
    if (SMTS_Event.header[PTPLib::common::Command.LEMMAS] != "0")
        clauses_request = (uint32_t) stoi(SMTS_Event.header[PTPLib::common::Command.LEMMAS].substr(1));

    std::vector<Node *> node_path;
    node_path.push_back(&this->lemmas[SMTS_Event.header[PTPLib::common::Param.NAME]]);
    std::string node_code = SMTS_Event.header[PTPLib::common::Param.NODE].substr(1, SMTS_Event.header[PTPLib::common::Param.NODE].size() - 2);
    node_code.erase(std::remove(node_code.begin(), node_code.end(), ' '), node_code.end());
    std::string const delimiter = ",";
    size_t beg, pos = 0;
    int counter = 0;
    while ((beg = node_code.find_first_not_of(delimiter, pos)) != std::string::npos)
    {
        pos = node_code.find_first_of(delimiter, beg + 1);
        if (counter % 2 == 1) {
            int index = stoi(node_code.substr(beg, pos - beg));
            while ((unsigned int)index >= node_path.back()->children.size()) {
                node_path.back()->children.push_back(new Node);
            }
            node_path.push_back(node_path.back()->children[index]);
        }
        counter++;
    }
}


