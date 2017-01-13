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


const char *SolverProcess::solver = "Spacer";

z3::context context;
z3::fixedpoint fixedpoint(context);
std::function<void(const std::vector<net::Lemma> &)> this_push;
std::function<void(std::vector<net::Lemma> &)> this_pull;

void push(Z3_fixedpoint_lemma_set s) {
    std::vector<net::Lemma> lemmas;
    Z3_fixedpoint_lemma *lemma;
    while ((lemma = Z3_fixedpoint_lemma_pop(context, s))) {
        lemmas.push_back(net::Lemma(std::to_string(lemma->level) + " " + lemma->str, 0));
        free(lemma);
    }
    this_push(lemmas);
}

void pull(Z3_fixedpoint_lemma_set s) {
    std::vector<net::Lemma> lemmas;
    this_pull(lemmas);
    Z3_fixedpoint_lemma l;
    for (net::Lemma &lemma:lemmas) {
        std::istringstream is(lemma.smtlib);
        is >> l.level;
        l.str = std::string(std::istreambuf_iterator<char>(is), {}).c_str();
        Z3_fixedpoint_lemma_push(context, s, &l);
    }
}

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
    z3::params p(context);
    p.set(":engine", context.str_symbol("spacer"));

    try {
        for (auto &pair:this->header) {
            if (pair.first.substr(0, 10) == "parameter.") {

                if (pair.second == "true")
                    p.set(pair.first.substr(10).c_str(), true);
                else if (pair.second == "false")
                    p.set(pair.first.substr(10).c_str(), false);
                else {
                    try {
                        p.set(pair.first.substr(10).c_str(), (unsigned) stoi(pair.second));
                    }
                    catch (std::exception) {
                        p.set(pair.first.substr(10).c_str(), context.str_symbol(pair.second.c_str()));
                    }
                }

            } else if (pair.first.substr(0, 6) == "trace.") {
                Z3_enable_trace(pair.first.substr(6).c_str());
            } else if (pair.first == "verbose") {
                Z3_global_param_set("verbose", pair.second.c_str());
            }
        }
        fixedpoint.set(p);
    }
    catch (z3::exception &ex) { // i'm not sending the msg because it's too long
        this->error("cannot set parameters");
    }
}

void SolverProcess::solve() {

    Z3_fixedpoint_set_lemma_pull_callback(context, fixedpoint, pull);
    Z3_fixedpoint_set_lemma_push_callback(context, fixedpoint, push);

    z3::solver solver(context);
    std::string smtlib = this->instance;

    while (true) {
        Z3_ast_vector v = Z3_fixedpoint_from_string(context, fixedpoint, (smtlib + this->header["query"]).c_str());
        //unsigned size = Z3_ast_vector_size(context, v);
        Z3_ast a = Z3_ast_vector_get(context, v, 0);

        Z3_lbool res = Z3_fixedpoint_query(context, fixedpoint, a);

        net::Header header;
        z3::stats statistics(context, Z3_fixedpoint_get_statistics(context, fixedpoint));
        for (uint32_t i = 0; i < statistics.size(); i++) {
            header["statistics." + statistics.key(i)] =
                    statistics.is_uint(i) ?
                    std::to_string(statistics.uint_value(i)) :
                    std::to_string(statistics.double_value(i));
        }
        if (res == Z3_L_TRUE)
            this->report(Status::sat, header);
        else if (res == Z3_L_FALSE)
            this->report(Status::unsat, header);

        Task t = this->wait();
        switch (t.command) {
            case Task::incremental:
                smtlib = t.smtlib;
                break;
            case Task::resume:
                smtlib.clear();
                break;
            default:
                return;
        }
    }
}

void SolverProcess::interrupt() {
    context.interrupt();
}

void SolverProcess::partition(uint8_t) {}