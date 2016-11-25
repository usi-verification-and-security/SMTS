//
// Created by Matteo on 10/12/15.
//

#include <unistd.h>
#include <fcntl.h>
#include <random>
#include <iostream>
#include "lib/Logger.h"
#include "client/SolverProcess.h"
#include "z3++.h"
#include "fixedpoint.h"


z3::context context;
std::function<void(const std::vector<net::Lemma> &)> this_push;
std::function<void(std::vector<net::Lemma> &)> this_pull;

void push(Z3_fixedpoint_lemma_set s) {
    std::vector<net::Lemma> lemmas;
    char *l;
    while ((l = (char *) Z3_fixedpoint_lemma_pop(context, s))) {
        lemmas.push_back(net::Lemma(l, 0));
        //std::cout << l << "\n";
        free(l);
    }
    this_push(lemmas);
}

void pull(Z3_fixedpoint_lemma_set s) {
    std::vector<net::Lemma> lemmas;
    this_pull(lemmas);
    for (net::Lemma &l:lemmas) {
        Z3_fixedpoint_lemma_push(context, s, l.smtlib.c_str());
    }
}

const char *SolverProcess::solver = "Spacer";

void SolverProcess::init() {
    FILE *file = fopen("/dev/null", "w");
    dup2(fileno(file), fileno(stdout));
    dup2(fileno(file), fileno(stderr));
    fclose(file);
    this_push = [&](const std::vector<net::Lemma> &l) {
        this->lemma_push(l);
    };
    this_pull = [&](std::vector<net::Lemma> &l) {
        this->lemma_pull(l);
    };
    if (this->header.count("lemmas") == 0) {
        this->header["lemmas"] = std::to_string(1000);
    }
}

void SolverProcess::solve() {
    z3::fixedpoint f(context);
    z3::params p(context);
    p.set(":engine", context.str_symbol("spacer"));
    p.set(":xform.slice", false);
    p.set(":xform.inline_eager", false);
    p.set(":xform.inline_linear", false);
    f.set(p);

    Z3_fixedpoint_set_lemma_pull_callback(context, f, pull);
    Z3_fixedpoint_set_lemma_push_callback(context, f, push);

    z3::solver solver(context);
    char *smtlib = (char *) this->instance.c_str();

    while (true) {
        Z3_ast_vector v = Z3_fixedpoint_from_string(context, f, smtlib);
        Z3_ast a = Z3_ast_vector_get(context, v, 0);

        Z3_lbool res = Z3_fixedpoint_query(context, f, a);
        z3::stats statistics(context, Z3_fixedpoint_get_statistics(context, f));
        for (uint32_t i = 0; i < statistics.size(); i++) {
            this->header["statistics." + statistics.key(i)] =
                    statistics.is_uint(i) ?
                    std::to_string(statistics.uint_value(i)) :
                    std::to_string(statistics.double_value(i));
        }

        if (res == Z3_L_TRUE)
            this->report(Status::sat);
        else if (res == Z3_L_FALSE)
            this->report(Status::unsat);
        else
            this->report(Status::unknown);
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
    context.interrupt();
}

void SolverProcess::partition(uint8_t) {}