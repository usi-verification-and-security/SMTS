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

void SolverProcess::cleanSolverState() {
    delete config;
    config = nullptr;
    delete splitterInterpret;
    splitterInterpret = nullptr;
    result = s_Undef;
}

SolverProcess::Result SolverProcess::solve(PTPLib::net::SMTS_Event SMTS_event, bool shouldUpdateSolverAddress) {

    result = s_Undef;
    getScatterSplitter().resetSplitType();
    if (log_enabled) {
        synced_stream.println(log_enabled ? PTPLib::common::Color::FG_Green : PTPLib::common::Color::FG_DEFAULT,
                              "[t SEARCH ] -> ", "CURRENT SOLVER BRANCH: ",
                              SMTS_event.header.at(PTPLib::common::Param.NODE),
                              " QUERY: ", SMTS_event.header.at(PTPLib::common::Param.QUERY));
        synced_stream.println(log_enabled ? PTPLib::common::Color::FG_Green : PTPLib::common::Color::FG_DEFAULT,
                              "[t SEARCH ] -> ", "SMT2 SCRIPT: ",
                              SMTS_event.body + SMTS_event.header.at(PTPLib::common::Param.QUERY));
    }
    assert(not SMTS_event.header.at(PTPLib::common::Param.QUERY).empty());
    sstat res = splitterInterpret->interpSMTContent((char *) (SMTS_event.body + SMTS_event.header.at(PTPLib::common::Param.QUERY)).c_str(),
    (shouldUpdateSolverAddress ? SMTS_event.header.at(PTPLib::common::Param.NODE).substr(1,
                                                                                         SMTS_event.header.at(PTPLib::common::Param.NODE).size() - 2) : ""));
    if (log_enabled) {
        int SearchCounter = (((ScatterSplitter &) getMainSplitter().getSMTSolver()).getSearchCounter());
        std::string option = "(set-option :solver-limit " + to_string(SearchCounter) + ")";
        if (not base_instance.empty())
            base_instance = std::string("(set-option :scatter-split)") + "\n" + base_instance;
        Logger::build_SolverInputPath(false, true,
                                      option + "\n" + base_instance + "\n" + SMTS_event.body + "\n" +
                                      SMTS_event.header.at(PTPLib::common::Param.QUERY),
                                      to_string(get_SMTS_socket().get_local()), getpid());
        base_instance.clear();
    }
    if (res == s_Undef)
        return SolverProcess::Result::UNKNOWN;
    else if (res == s_True)
        return SolverProcess::Result::SAT;
    else if (res == s_False)
        return SolverProcess::Result::UNSAT;
    else if (res == s_Error)
        return SolverProcess::Result::ERROR;

    net::Report::error(get_SMTS_socket(), SMTS_event.header, "parser error");
}

volatile sig_atomic_t shutdown_flag = 1;
void cleanupRoutine(int signal_number) {
    shutdown_flag = 0;
}

void SolverProcess::partition(PTPLib::net::SMTS_Event & SMTS_Event, uint8_t n) {
    if (not log_enabled) {
        FILE * file = fopen("/dev/null", "w");
        dup2(fileno(file), fileno(stdout));
        dup2(fileno(file), fileno(stderr));
        fclose(file);
    }
//    fork() returns -1 if it fails, and if it succeeds, it returns the forked child's pid in the parent, and 0 in the child.
    forked_partitionId = fork();
    if (forked_partitionId == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (forked_partitionId > 0) {
        forked = true;
        return;
    }
    std::thread t_handle_orphant([&] {
        while (true) {
            sleep(1);
            if (getppid() == 1)
                exit(EXIT_SUCCESS);
        }
    });

    struct sigaction sigterm_action;
    memset(&sigterm_action, 0, sizeof(sigterm_action));
    sigterm_action.sa_handler = &cleanupRoutine;
    sigterm_action.sa_flags = 0;

    if (sigfillset(&sigterm_action.sa_mask) != 0)
    {
        perror("sigfillset");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGTERM, &sigterm_action, NULL) != 0)
    {
        perror("sigaction SIGTERM");
        exit(EXIT_FAILURE);
    }
    getChannel().clearClauseShareMode();
    std::vector<std::string> partitions;
    int searchCounter = (((ScatterSplitter &) getMainSplitter().getSMTSolver()).getSearchCounter());
    std::string statusInfo = getMainSplitter().getConfig().getInfo(":status").toString();

    const char *msg;
    if ( not (
            getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_num, SMTOption(int(n)),msg)                   and
            getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_units, SMTOption(spts_search_counter), msg)   and
            getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_inittune, SMTOption(1), msg)         and
            getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_midtune, SMTOption(1), msg)
             )
        )
    {
        net::Report::report(get_SMTS_socket(), partitions, to_string(searchCounter), statusInfo, SMTS_Event, msg);
    }
    else {
        try {
            getScatterSplitter().setSplitTypeScatter();
            sstat status = getMainSplitter().solve();
            if (status == s_Undef) {
                partitions = getMainSplitter().getPartitionClauses();
                net::Report::report(get_SMTS_socket(), partitions, to_string(searchCounter), statusInfo, SMTS_Event);
            }
            else if (status == s_True) {
                synced_stream.println_bold(log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                                           "[ t ", __func__, "] -> ", " Partition Report sat ", SMTS_Event.header.at(PTPLib::common::Param.NODE));
                net::Report::report(get_SMTS_socket(), SMTS_Event.header, SolverProcess::resultToString( Result::SAT));
            }
            else if (status == s_False) {
                synced_stream.println_bold(log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                                           "[ t ", __func__, "] -> ", " Partition Report unsat ", SMTS_Event.header.at(PTPLib::common::Param.NODE));
                net::Report::report(get_SMTS_socket(), SMTS_Event.header, SolverProcess::resultToString( Result::UNSAT));
            }
            else {
                net::Report::report(get_SMTS_socket(), partitions, to_string(searchCounter), statusInfo, SMTS_Event, "error during partitioning");
            }
        }
        catch (std::exception & ex)
        {
            net::Report::error(get_SMTS_socket(), SMTS_Event.header, std::string(ex.what()));
            exit(EXIT_FAILURE);
        }
    }
    if (log_enabled) {
        fprintf(stdout,
                "; ============================[ Partition Statistics ]=================================================\n");
        fprintf(stdout,
                "; | SearchCounter |          SplitType         |          Child Process ID    | Time     | MEM USAGE | \n");
        fprintf(stdout,
                "; |           |                            |                              |          |           | \n");
        reportf("; %9d   | %8d                %8d           |           %8.3f s      | %6.3f MB\n", searchCounter,
                getScatterSplitter().getSplitTypeValue(), getpid(), cpuTime(), memUsed() / 1048576.0);
        fflush(stderr);
        fprintf(stdout,
                "; =====================================================================================================\n");
    }
    exit(EXIT_SUCCESS);
}


