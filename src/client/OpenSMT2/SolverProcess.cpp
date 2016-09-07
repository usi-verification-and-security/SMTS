//
// Created by Matteo on 10/12/15.
//

#include <unistd.h>
#include <string>
#include <iostream>
#include <random>
#include <client/SolverProcess.h>
#include "client/SolverProcess.h"
#include "OpenSMTSolver.h"
#include "SplitProcess.h"


using namespace opensmt;

const char *SolverProcess::solver = "OpenSMT2";

void SolverProcess::init() {
    FILE *file = fopen("/dev/null", "w");
    //dup2(fileno(file), fileno(stdout));
    dup2(fileno(file), fileno(stderr));
    fclose(file);

    if (this->header.count("seed") == 0) {
        std::uniform_int_distribution<uint32_t> randuint(0, 0xFFFFFF);
        std::random_device rd;
        this->header["seed"] = std::to_string(randuint(rd));
    }
    if (this->header.count("lemmas") == 0) {
        this->header["lemmas"] = std::to_string(1000);
    }
}

void SolverProcess::solve() {
    SMTConfig config;
    config.setRandomSeed(atoi(this->header["seed"].c_str()));
    OpenSMTInterpret interpret(this->header, this->lemmas, config);
    auto partition = [&](Task &task) {
        SplitProcess process(interpret.main_solver, task);
        std::map<std::string, std::string> header;
        std::string payload;
        process.reader()->read(header, payload);
        if (header.count("status"))
            return this->report((Status) header["status"][0]);
        header["name"] = this->header["name"];
        header["node"] = this->header["node"];
        this->writer()->write(header, payload);
        exit(0);
    };
    char *smtlib = (char *) this->instance.c_str();

    while (true) {
        interpret.interpFile(smtlib);
        interpret.f_exit = false;
        opensmt::stop = false;
        sstat status = interpret.main_solver->getStatus();

        if (status == s_True)
            this->report(Status::sat);
        else if (status == s_False)
            this->report(Status::unsat);
        else this->report(Status::unknown);

        wait:
        Task task = this->wait();
        switch (task.command) {
            case Task::incremental:
                smtlib = (char *) task.smtlib.c_str();
                break;
            case Task::partition:
                partition(task);
                goto wait;
            case Task::resume:
                smtlib = (char *) "(check-sat)";
                break;
        }
    }
}

void SolverProcess::interrupt() {
    opensmt::stop = true;
}