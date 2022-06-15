/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMTS_LEMMASERVER_LEMMASERVER_H
#define SMTS_LEMMASERVER_LEMMASERVER_H

#include "Lemma.h"
#include "Settings.h"
#include "Node.h"
#include "lib/net.h"
#ifdef SQLITE_IS_ON
#include "lib/sqlite3.h"
#endif

#include <PTPLib/net/Channel.hpp>
#include <PTPLib/threads/ThreadPool.hpp>

#include <unordered_map>
#include <ctime>

class LemmaServer : public net::Server {
private:
    bool send_again;
    std::shared_ptr<net::Socket> server;
    PTPLib::net::Channel channel;
#ifdef SQLITE_IS_ON
    std::shared_ptr<SQLite3::Connection> db;
#endif
    std::unordered_map<std::string, Node> lemmas;                            // name -> lemmas
    std::unordered_map<std::string, std::unordered_map<int, std::unordered_map<Lemma *, bool>>> solvers;  // name -> solver -> lemma -> t/f
    bool logEnabled = false;
    std::size_t lemmasSize = 0;

    PTPLib::threads::ThreadPool pool;

    void garbageCollect(std::size_t batchSize, std::string const & instanceName);

protected:
    void handle_accept(net::Socket const &);

    void handle_close(net::Socket &);

    void handle_event(net::Socket &, PTPLib::net::SMTS_Event &&);

    void handle_exception(net::Socket const &, const std::exception &);

    void lemma_worker(net::Socket &&, PTPLib::net::SMTS_Event &&);

    PTPLib::net::Channel & getChannel()             { return channel; };
    net::Socket const & getSMTS_serverSocket()    { return *server.get(); };

    void notify_reset();
    void memory_checker(int);

public:
    LemmaServer(uint16_t, const std::string &, const std::string &, bool send_again);
};

#endif
