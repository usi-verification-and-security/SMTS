//
// Created by Matteo on 10/12/15.
//

#include <unistd.h>
#include <fcntl.h>
#include <random>
#include <iostream>
#include "lib/Log.h"
#include "client/SolverProcess.h"
#include "z3++.h"


z3::context *context = nullptr;

const char *SolverProcess::solver = "Z3";

void SolverProcess::init() { }

void SolverProcess::solve() {
    ::context = new z3::context;
    z3::solver solver(*context);
    char *smtlib = (char *) this->instance.c_str();

    Z3_fixedpoint d = Z3_mk_fixedpoint(*context);

    Z3_params p = Z3_mk_params(*context);
    Z3_params_set_symbol(*context, p, Z3_mk_string_symbol(*context, "engine"), Z3_mk_string_symbol(*context, "spacer"));
    Z3_fixedpoint_set_params(*context, d, p);
    std::cout << "M\n";
    Z3_fixedpoint_set_push_callback(*context, d, nullptr);
    sleep(1);
    exit(0);

    while (true) {
        Z3_ast a = Z3_parse_smtlib2_string(*context, smtlib, 0, 0, 0, 0, 0, 0);
        z3::expr e(*context, a);
        solver.add(e);
        z3::check_result status = solver.check();
        if (status == z3::check_result::sat)
            this->report(Status::sat);
        else if (status == z3::check_result::unsat)
            this->report(Status::unsat);
        else this->report(Status::unknown);
        Task t = this->wait();
        switch (t.command) {
            case Task::incremental:
                smtlib = (char *) t.smtlib.c_str();
                break;
            case Task::resume:
                smtlib = (char *) "(check-sat)";
                break;
        }
    }
}

void SolverProcess::interrupt() {
    context->interrupt();
}

void SolverProcess::partition(uint8_t) { }