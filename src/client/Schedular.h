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
    PTPLib::net::Channel<PTPLib::net::SMTS_Event, PTPLib::net::Lemma> & channel;
    PTPLib::threads::ThreadPool &       thread_pool;
    SolverProcess *                     solver_process;
    net::Socket *                       SMTS_server_socket  = nullptr;
    net::Socket *                       Lemma_server_socket = nullptr;
    PTPLib::common::synced_stream &     synced_stream;

    mutable std::mutex                  lemma_mutex;
    bool                                log_enabled;
    std::string                         lemma_amount;

#ifdef VERBOSE_THREAD
    std::atomic<std::thread::id>        memory_thread_id;
    std::atomic<std::thread::id>        push_thread_id;
    std::atomic<std::thread::id>        pull_thread_id;
    std::atomic<std::thread::id>        communicator_thread_id;
#endif

    bool is_lemmaServer_sharing()        const       { return this->Lemma_server_socket != nullptr; }
    net::Socket & get_lemma_server()     const       { return  *Lemma_server_socket; }
    net::Socket & get_SMTS_server()      const       { return  *SMTS_server_socket; }

    void memory_checker(int max_memory);

    void worker(PTPLib::common::TASK tname, int seed, int td_min, int td_max);



    bool request_solver_toStop(PTPLib::net::SMTS_Event const & SMTS_event);

    void communicate_worker();

    inline void delete_solver_process() {
        delete this->solver_process;
        this->solver_process = nullptr;
    }

    void push_clause_worker(int seed, int n_min, int n_max);
    void lemmas_publish(std::unique_ptr<PTPLib::net::map_solverBranch_lemmas> const & lemmas, PTPLib::net::Header & header);
    void lemma_push(std::vector<PTPLib::net::Lemma> const & toPush_lemma, PTPLib::net::Header & header);

    void pull_clause_worker(int seed, int n_min, int n_max);
    bool read_lemma(std::vector<PTPLib::net::Lemma> & lemmas, PTPLib::net::Header & header);
    bool lemma_pull(std::vector<PTPLib::net::Lemma> &lemmas, PTPLib::net::Header &header);

    void periodic_clauseLearning_worker(int wait_duration);

    bool execute_event(std::unique_lock<std::mutex> & s_lk, PTPLib::net::SMTS_Event & smts_event, bool & shouldUpdateSolverAddress);

public:

    Schedular (PTPLib::threads::ThreadPool           & th_pool,
                PTPLib::common::synced_stream        & ss,
                PTPLib::net::Channel<PTPLib::net::SMTS_Event, PTPLib::net::Lemma> & ch,
                net::Socket                          & server,
                const bool                           & le)
    : channel                               (ch)
    , thread_pool                           (th_pool)
    , SMTS_server_socket                    (&server)
    , synced_stream                         (ss)
    , log_enabled                           (le)
     {}

    ~Schedular() = default;

    void push_to_pool(PTPLib::common::TASK tname, int seed = 0, int td_min = 0, int td_max = 0);

    inline void set_lemma_amount(std::string la)        { lemma_amount = la; }

    inline void set_log_enabled(bool le)                { log_enabled = le; }

    PTPLib::net::Channel<PTPLib::net::SMTS_Event, PTPLib::net::Lemma> & getChannel()          { return channel; }

    PTPLib::threads::ThreadPool & getPool()      { return thread_pool; }

    template<class T>
    bool queue_event(T && event) {
        bool reset = false;
        if (event.header[PTPLib::common::Param.COMMAND] == PTPLib::common::Command.STOP) {
            reset = true;
            getChannel().push_front_event(std::forward<T>(event));
        }
        else {
            if (event.header[PTPLib::common::Param.COMMAND] == PTPLib::common::Command.SOLVE)
                event.header[PTPLib::common::Param.COMMAND] = "resume";
            getChannel().push_back_event(std::forward<T>(event));
        }

        getChannel().notify_all();
        return reset;
    };

    void notify_reset();

    void set_LemmaServer_socket(net::Socket * lemma_server_socket) { Lemma_server_socket = lemma_server_socket; }

    bool preProcess_instance(PTPLib::net::SMTS_Event & smts_event);

};
#endif