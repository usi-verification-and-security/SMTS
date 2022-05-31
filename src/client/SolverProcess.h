/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMTS_CLIENT_SOLVERPROCESS_H
#define SMTS_CLIENT_SOLVERPROCESS_H

#include "lib/net.h"

#include <PTPLib/net/Channel.hpp>
#include <PTPLib/common/Printer.hpp>
#include <PTPLib/common/Lib.hpp>

#include <ctime>
#include <unistd.h>
#include <thread>

class SolverProcess  {
    friend class Schedular;

public:
    SolverProcess(PTPLib::common::synced_stream & ss, net::Socket * s, PTPLib::net::Channel& ch)
    : synced_stream(ss)
    , channel(ch)
    , SMTS_server_socket(s)
    {}

    static std::string solver;

    enum class Result { SAT, UNSAT, UNKNOWN, ERROR };

    // here the module can edit class fields
    SolverProcess::Result init(PTPLib::net::SMTS_Event & SMTS_Event);

    void partition(PTPLib::net::SMTS_Event & SMTS_Event, uint8_t);

    Result solve(PTPLib::net::SMTS_Event SMTS_event, bool shouldUpdateSolverAddress);

    void add_constraint(std::unique_ptr<PTPLib::net::map_solver_clause> const & clauses, std::string & branch);

    net::Socket & get_SMTS_socket() const   { return *SMTS_server_socket; }

    ~SolverProcess() {
        if (forked)
            kill_partition_process();
        cleanSolverState();
    }

private:
    PTPLib::common::synced_stream & synced_stream;
    PTPLib::net::Channel & channel;
    net::Socket * SMTS_server_socket;
    pid_t forked_partitionId;
    bool log_enabled = false;
    bool forked = false;

    void kill_partition_process();

    // Get CNF corresponding to a particular solver
    void getCnfClauses(PTPLib::net::SMTS_Event & smtsEvent);

    void getCnfLearnts(PTPLib::net::Header & header);

    void cleanSolverState();

    PTPLib::net::Channel& getChannel()  {  return channel; };

    static std::string resultToString(SolverProcess::Result res) {
        if (res == SolverProcess::Result::UNKNOWN)
            return "unknown";
        else if (res == SolverProcess::Result::SAT)
            return "sat";
        else if (res == SolverProcess::Result::UNSAT)
            return "unsat";
        else if (res == SolverProcess::Result::ERROR)
            return "error";
        else return "undefined";
    };

    bool isPrefix(std::string_view prefix, std::string_view full) {
        if (prefix.size() > full.size())
            return false;
        return prefix == full.substr(0, prefix.size());
    }

};

#endif
