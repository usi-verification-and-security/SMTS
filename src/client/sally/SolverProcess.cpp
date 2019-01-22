//
// Created by Martin Blicha on 2019-01-15.
//

#include "lib/Logger.h"
#include "lib/lib.h"
#include "client/SolverProcess.h"
#include <sally_api.h>

#include <memory>

const char *SolverProcess::solver = "SALLY";

using namespace sally;

struct ContextWrapper {
    sally_context ctx;
    ~ContextWrapper() {
        delete_context(ctx);
    }
};

void SolverProcess::init() {
    std::map<std::string, std::string> opts;
    opts["engine"] = "pdkind";
    opts["solver-logic"] = "QF_LRA";
    opts["solver"] = "y2o2";
    for (auto &key:this->header.keys(net::Header::parameter)) {
        const std::string & value = this->header.get(net::Header::parameter, key);
        opts[key] = value;
    }
    sally_context ctx = create_context(opts);
    ContextWrapper* wrapper = new ContextWrapper();
    wrapper->ctx = ctx;
    this->state.reset(wrapper);

//    Z3_fixedpoint_add_callback(state.context, state.fixedpoint, &state, new_lemma_eh, predecessor_eh, unfold_eh);
}

void SolverProcess::solve() {
    ContextWrapper* wrapper = static_cast<ContextWrapper* >(this->state.get());
    sally_context ctx = wrapper->ctx;
    std::string instance = this->instance + this->header["query"];
    while (true) {
        // Capture std::cout
        std::stringstream buffer;
        std::streambuf * old = std::cout.rdbuf(buffer.rdbuf());
        run_on_mcmt_string(instance, ctx);
        // get the result
        std::string res = buffer.str();
//        std::string res = "";

            if (res.rfind("valid") == 0) {
                report(Status::unsat);
            }
            else if (res.rfind("invalid") == 0) {
                report(Status::sat);
            }
            else {
                error(res);
            }
        std::cout.rdbuf(old);

        Task t = this->wait();
        switch (t.command) {
            case Task::incremental:
                instance = t.smtlib;
                break;
            case Task::resume:
                instance.clear();
                break;
            default:
                throw std::logic_error{"Unreachable!"};
        }
    }
}

void SolverProcess::interrupt() {

}

void SolverProcess::getCnfClauses(net::Header &header, const std::string &payload) {
    this->report(header, header["command"], "");
}

void SolverProcess::getCnfLearnts(net::Header &header) {
    this->report(header, header["command"], "");
}

void SolverProcess::partition(uint8_t) {}