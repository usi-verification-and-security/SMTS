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

#include <map>
#include <unordered_map>
#include <ctime>

class LemmaServer : public net::Server {
private:
    bool send_again;
    std::shared_ptr<net::Socket> server;
#ifdef SQLITE_IS_ON
    std::shared_ptr<SQLite3::Connection> db;
#endif
    std::unordered_map<std::string, Node> lemmas;                            // name -> lemmas
    std::unordered_map<std::string, std::unordered_map<net::Socket *, std::map<Lemma *, bool>>> solvers;  // name -> solver -> lemma -> t/f
    bool logEnabled = false;

protected:
    void handle_accept(net::Socket const &);

    void handle_close(net::Socket &);

    void handle_exception(net::Socket const &, const std::exception &);

public:
    LemmaServer(uint16_t, const std::string &, const std::string &, bool send_again);
};

#endif
