/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#include "SolverServer.h"
#include "lib/net/Report.h"
#include "lib/Logger.h"

#include <iostream>

void ReportSolverType(net::Socket const &);

SolverServer::SolverServer(net::Address const & server)
: net::Server()
, SMTSServer_socket(server)
, thread_pool( __func__ ,  6)
, schedular(thread_pool, synced_stream, channel, SMTSServer_socket, log_enabled) {
    ReportSolverType(get_SMTS_server_socket());
    channel.setParallelMode();
    this->add_socket(&SMTSServer_socket);
}

void ReportSolverType(net::Socket const & SMTS_server)
{
    PTPLib::net::Header header;
    header[PTPLib::common::Param.SOLVER] = SolverProcess::solver;
    SMTS_server.write(PTPLib::net::SMTS_Event(std::move(header)));
}
