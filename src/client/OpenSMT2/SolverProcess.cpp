//
// Created by Matteo on 10/12/15.
//

#include <unistd.h>
#include <string>
#include <iostream>
#include <random>
#include "client/SolverProcess.h"
#include "OpenSMTSolver.h"


using namespace opensmt;

OpenSMTInterpret *interpret = nullptr;

const char *SolverProcess::solver = "OpenSMT2";

void SolverProcess::init() {
    FILE *file = fopen("/dev/null", "w");
    //dup2(fileno(file), fileno(stdout));
    dup2(fileno(file), fileno(stderr));
    fclose(file);

    if (this->header.count("config.seed") == 0) {
        std::uniform_int_distribution<uint32_t> randuint(0, 0xFFFFFF);
        std::random_device rd;
        this->header["config.seed"] = std::to_string(randuint(rd));
    }
}

void SolverProcess::solve() {
    SMTConfig config;
    config.setRandomSeed(atoi(this->header["config.seed"].c_str()));
    auto lemma_push = [&](const std::vector<net::Lemma> &lemmas) {
        this->lemma_push(lemmas);
    };
    auto lemma_pull = [&](std::vector<net::Lemma> &lemmas) {
        this->lemma_pull(lemmas);
    };
    interpret = new OpenSMTInterpret(this->header, lemma_push, lemma_pull, config);
    char *smtlib = (char *) this->instance.c_str();

    while (true) {
        interpret->interpFile(smtlib);
        interpret->f_exit = false;
        opensmt::stop = false;
        sstat status = interpret->main_solver->getStatus();

        if (status == s_True)
            this->report(Status::sat);
        else if (status == s_False)
            this->report(Status::unsat);
        else
            this->report(Status::unknown);

        Task task = this->wait();
        switch (task.command) {
            case Task::incremental:
                smtlib = (char *) task.smtlib.c_str();
                break;
            case Task::resume:
                smtlib = (char *) "(check-sat)";
                break;
        }
    }
}

void SolverProcess::interrupt() {
    opensmt::stop = true;
}

void SolverProcess::partition(uint8_t n) {
    pid_t pid = fork();
    if (pid != 0)
        return;
    std::vector<std::string> partitions;
    const char *msg;
    if (!(
            interpret->main_solver->getConfig().setOption(SMTConfig::o_sat_split_num,
                                                          SMTOption(int(n)),
                                                          msg) &&
            interpret->main_solver->getConfig().setOption(SMTConfig::o_sat_split_type, SMTOption(spts_lookahead),
                                                          msg) &&
            interpret->main_solver->getConfig().setOption(SMTConfig::o_sat_split_units, SMTOption(spts_time), msg) &&
            interpret->main_solver->getConfig().setOption(SMTConfig::o_sat_split_inittune, SMTOption(double(2)), msg) &&
            interpret->main_solver->getConfig().setOption(SMTConfig::o_sat_split_midtune, SMTOption(double(2)), msg) &&
            interpret->main_solver->getConfig().setOption(SMTConfig::o_sat_split_asap, SMTOption(1), msg))) {
        this->report(partitions, msg);
    } else {
        sstat status = interpret->main_solver->solve();
        if (status == s_Undef) {
            vec<SplitData> &splits = interpret->main_solver->getSMTSolver().splits;
            for (int i = 0; i < splits.size(); i++) {
                vec<vec<PtAsgn>> constraints;
                splits[i].constraintsToPTRefs(constraints);
                vec<PTRef> clauses;
                for (int j = 0; j < constraints.size(); j++) {
                    vec<PTRef> clause;
                    for (int k = 0; k < constraints[j].size(); k++) {
                        PTRef pt =
                                constraints[j][k].sgn == l_True ?
                                constraints[j][k].tr :
                                interpret->main_solver->getLogic().mkNot(constraints[j][k].tr);
                        clause.push(pt);
                    }
                    clauses.push(interpret->main_solver->getLogic().mkOr(clause));
                }
                char *str = interpret->main_solver->getTHandler().getLogic().
                        printTerm(interpret->main_solver->getLogic().mkAnd(clauses), false, true);
                partitions.push_back(str);
                free(str);
            }
            this->report(partitions);
        } else if (status == s_True)
            this->report(Status::sat);
        else if (status == s_False)
            this->report(Status::unsat);
        else
            this->report(partitions, "unknown status after partition");
    }
    exit(0);
}
