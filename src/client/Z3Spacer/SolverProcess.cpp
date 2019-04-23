//
// Author: Matteo Marescotti
//

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <random>
#include <iostream>
#include "lib/Logger.h"
#include "client/SolverProcess.h"
#include "z3++.h"


const char *SolverProcess::solver = "Spacer";

struct State {
    z3::context context;
    z3::fixedpoint fixedpoint;
    SolverProcess *solverProcess;

    State(SolverProcess *solverProcess) : fixedpoint(context), solverProcess(solverProcess) {}
};

std::vector<net::Lemma> lemmas;

void new_lemma_eh(void *_state, Z3_ast lemma, unsigned level) {
    State &state = *static_cast<State *>(_state);
    z3::solver solver(state.context);
    solver.add(z3::expr(state.context, lemma));
    std::ostringstream ss;
    ss << std::to_string(level) << " " << solver;
    lemmas.emplace_back(net::Lemma(ss.str(), 0));
}

void predecessor_eh(void *_state) {
    State &state = *static_cast<State *>(_state);
    std::vector<net::Lemma> lemmas;
    unsigned level;
    state.solverProcess->lemma_pull(lemmas);
    for (net::Lemma &lemma:lemmas) {
        std::istringstream is(lemma.smtlib);
        is >> level;
        z3::expr_vector v = state.context.parse_string(std::string(std::istreambuf_iterator<char>(is), {}).c_str());
        for (unsigned i = 0; i < v.size(); i++) {
            Z3_fixedpoint_add_constraint(state.context, state.fixedpoint, v[i], level);
        }
    }
}

void unfold_eh(void *_state) {
    State &state = *static_cast<State *>(_state);
    state.solverProcess->lemma_push(lemmas);
    lemmas.clear();
}


void SolverProcess::init() {
    this->state.reset(new State(this));
    State &state = *static_cast<State *>(this->state.get());
    FILE *file = fopen("/dev/null", "w");
    //dup2(fileno(file), fileno(stdout));
    //dup2(fileno(file), fileno(stderr));
    fclose(file);
    z3::params p(state.context);

    try {
        for (auto &key:this->header.keys(net::Header::parameter)) {
            const std::string &value = this->header.get(net::Header::parameter, key);
            if (key == "trace.") {
                Z3_enable_trace(key.substr(6).c_str());
            } else if (key == "verbose") {
                Z3_global_param_set("verbose", value.c_str());
            } else {
                if (value == "true")
                    p.set(key.c_str(), true);
                else if (value == "false")
                    p.set(key.c_str(), false);
                else {
                    try {
                        p.set(key.c_str(), (unsigned) stoi(value));
                    }
                    catch (std::exception) {
                        p.set(key.c_str(), state.context.str_symbol(value.c_str()));
                    }
                }
            }

        }
        p.set(":engine", state.context.str_symbol("spacer"));
        state.fixedpoint.set(p);
    }
    catch (z3::exception &ex) { // i'm not sending the msg because it's too long
        this->error("cannot set parameters");
    }
    Z3_fixedpoint_add_callback(state.context, state.fixedpoint, &state, new_lemma_eh, predecessor_eh, unfold_eh);
}

void SolverProcess::solve() {
    State &state = *static_cast<State *>(this->state.get());
    z3::solver solver(state.context);
    std::string smtlib = this->instance;

    while (true) {
        Z3_ast_vector v = Z3_fixedpoint_from_string(state.context, state.fixedpoint,
                                                    (smtlib + this->header["query"]).c_str());
        Z3_ast a = Z3_ast_vector_get(state.context, v, 0);

        Z3_lbool res;
        try {
            res = Z3_fixedpoint_query(state.context, state.fixedpoint, a);
        } catch (z3::exception &e) {
            this->error(std::string("Z3 exception: ") + e.msg());
        }

        z3::stats statistics(state.context, Z3_fixedpoint_get_statistics(state.context, state.fixedpoint));
        for (uint32_t i = 0; i < statistics.size(); i++) {
            this->header.set(net::Header::statistic, statistics.key(i),
                             statistics.is_uint(i) ?
                             std::to_string(statistics.uint_value(i)) : std::to_string(statistics.double_value(i))
            );
        }
        if (res == Z3_L_TRUE)
            this->report(Status::sat);
        else if (res == Z3_L_FALSE)
            this->report(Status::unsat);

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
    (*static_cast<State *>(this->state.get())).context.interrupt();
}

void SolverProcess::getCnfClauses(net::Header &header, const std::string &payload) {
    this->report(header, header["command"], "");
}

void SolverProcess::getCnfLearnts(net::Header &header) {
    this->report(header, header["command"], "");
}

void SolverProcess::partition(uint8_t) {}
