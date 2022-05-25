/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#include "client/SolverProcess.h"
#include "lib/net/Report.h"
#include "lib/Logger.h"

#include <SplitterInterpret.h>
#include <ReportUtils.h>

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include <iostream>

std::string SolverProcess::solver = PTPLib::common::Param.OPENSMT2;

SplitterInterpret *         splitterInterpret;
SMTConfig    *              config;
sstat                       result;
std::string                 base_instance;

inline MainSplitter & getMainSplitter() {
    return dynamic_cast<MainSplitter&>(splitterInterpret->getMainSolver());
};

inline ScatterSplitter & getScatterSplitter() {
    return dynamic_cast<ScatterSplitter&>(getMainSplitter().getSMTSolver());
}

void  SolverProcess::init(PTPLib::net::SMTS_Event & SMTS_Event) {
    const char *msg;
    static const char *default_split = SMTConfig::o_sat_scatter_split;
    static const char *default_seed = "0";

    if (SMTS_Event.header.get(PTPLib::net::parameter, PTPLib::common::Param.SPLIT_TYPE).size() > 0 and
            SMTS_Event.header.get(PTPLib::net::parameter, PTPLib::common::Param.SPLIT_TYPE) != SMTConfig::o_sat_lookahead_split and
            SMTS_Event.header.get(PTPLib::net::parameter, PTPLib::common::Param.SPLIT_TYPE) != SMTConfig::o_sat_scatter_split)
    {
        net::Report::warning(get_SMTS_socket(), SMTS_Event.header,"bad parameter.split-type: '" +
            SMTS_Event.header.get(PTPLib::net::parameter, PTPLib::common::Param.SEED) + "'. using default (" + default_split +")");
    }
    if (SMTS_Event.header.get(PTPLib::net::parameter, PTPLib::common::Param.SPLIT_TYPE).size() == 0) {
        SMTS_Event.header.set(PTPLib::net::parameter, PTPLib::common::Param.SPLIT_TYPE, default_split);
    }

    if ( not (config = new SMTConfig()))
        throw PTPLib::common::Exception(__FILE__, __LINE__, ";SMTConfig: out of memory");

    config->setRandomSeed(atoi(SMTS_Event.header.get(PTPLib::net::parameter, "seed").c_str()));
    config->setOption(SMTConfig::o_sat_scatter_split,
                     SMTOption(SMTS_Event.header.get(PTPLib::net::parameter, PTPLib::common::Param.SPLIT_TYPE).c_str()), msg);
    if (SMTS_Event.header.count(PTPLib::common::Param.SPLIT_PREFERENCE)) {
        config->setOption(SMTConfig::o_sat_split_preference, SMTOption(SMTS_Event.header.at(PTPLib::common::Param.SPLIT_PREFERENCE).c_str()), msg);
    }
    config->setOption(SMTConfig::o_sat_split_units, SMTOption(spts_search_counter), msg);
    config->setOption(SMTConfig::o_sat_split_inittune, SMTOption(INT_MAX), msg);

    if (not (splitterInterpret = new SplitterInterpret(*config, getChannel())))
        throw PTPLib::common::Exception(__FILE__, __LINE__, ";SplitterInterpret: out of memory");

    splitterInterpret->interpSMTContent((char *) SMTS_Event.body.c_str());

    if (log_enabled)
        getScatterSplitter().set_syncedStream(synced_stream);
        base_instance = SMTS_Event.body;
        Logger::build_SolverInputPath(true, true, "(set-option :random-seed " + SMTS_Event.header.get(PTPLib::net::parameter, "seed") + ")"
                                  "\n" + std::string("(set-option :split-units time)") + "\n" + std::string("(set-option :split-init-tune "+ to_string(DBL_MAX) + ")"),
                                  to_string(get_SMTS_socket().get_local()), getpid());
}
