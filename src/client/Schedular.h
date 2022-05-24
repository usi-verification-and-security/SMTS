/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMTS_CLIENT_SCHEDULAR_H
#define SMTS_CLIENT_SCHEDULAR_H

#include "SolverProcess.h"

#include <PTPLib/net/Channel.hpp>
#include <PTPLib/net/Header.hpp>
#include <PTPLib/common/PartitionConstant.hpp>
#include <PTPLib/threads/ThreadPool.hpp>

class Schedular {
private:
    PTPLib::net::Channel &              channel;
    PTPLib::threads::ThreadPool &       thread_pool;
    SolverProcess *                     solver_process;
    net::Socket *                       SMTS_server_socket  = nullptr;
    net::Socket *                       Lemma_server_socket = nullptr;
    PTPLib::common::synced_stream &     synced_stream;
    PTPLib::common::StoppableWatch      timer;

    mutable std::mutex                  lemma_mutex;
    bool                                log_enabled;
    std::string                         lemma_amount;

#ifdef VERBOSE_THREAD
    std::atomic<std::thread::id>        memory_thread_id;
    std::atomic<std::thread::id>        push_thread_id;
    std::atomic<std::thread::id>        pull_thread_id;
    std::atomic<std::thread::id>        communicator_thread_id;
#endif

    inline bool is_lemmaServer_sharing()        const       { return this->Lemma_server_socket != nullptr; }
    inline net::Socket & get_lemma_server()     const       { return  *Lemma_server_socket; }
    inline net::Socket & get_SMTS_server()      const       { return  *SMTS_server_socket; }

    void memory_checker(int max_memory);

public:

    Schedular (PTPLib::threads::ThreadPool           & th_pool,
                PTPLib::common::synced_stream        & ss,
                PTPLib::net::Channel                 & ch,
                net::Socket                          & server,
                const bool                           & le)
    : channel                               (ch)
    , thread_pool                           (th_pool)
    , synced_stream                         (ss)
    , SMTS_server_socket                    (&server)
    , log_enabled                           (le)
     {}

    ~Schedular() = default;

};
#endif