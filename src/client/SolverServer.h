/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMTS_CLIENT_SOLVERSERVER_H
#define SMTS_CLIENT_SOLVERSERVER_H

#include "lib/net.h"
#include "SolverProcess.h"
#include "Schedular.h"

#include <PTPLib/threads/ThreadPool.hpp>

class SolverServer : public net::Server {

private:
    bool log_enabled = false;

    net::Socket SMTSServer_socket;

    PTPLib::threads::ThreadPool thread_pool;

    Schedular schedular;

    std::shared_ptr<net::Socket> lemmaServer_socket = nullptr;

    PTPLib::net::Channel<PTPLib::net::SMTS_Event, PTPLib::net::Lemma> channel;

    PTPLib::common::synced_stream synced_stream;

    struct {
        int lemma_push_min = 0;
        int lemma_push_max = 0;
        int lemma_pull_min = 0;
        int lemma_pull_max = 0;
        int seed = 0;
    } lemma_stat;

    void stop_schedular();

    void initiate_lemma_server(PTPLib::net::SMTS_Event & SMTS_Event);

    void push_lemma_workers();

    void setUpThreadArch(PTPLib::net::SMTS_Event const & SMTS_Event);

protected:
    void handle_close(net::Socket & SMTSServer_socket);

    void handle_exception(net::Socket const & SMTSServer_socket, const std::exception &);

    void handle_event(net::Socket & SMTSServer_socket, PTPLib::net::SMTS_Event && SMTS_event);

public:
    PTPLib::net::Channel<PTPLib::net::SMTS_Event, PTPLib::net::Lemma> & getChannel()   { return channel; };
    net::Socket const & get_SMTS_server_socket()    { return  this->SMTSServer_socket; };

    SolverServer(net::Address const & server);

    ~SolverServer() = default;
};


#endif
