//
// Created by Martin Blicha on 2019-01-15.
//

#include "lib/Logger.h"
#include "lib/lib.h"
#include "client/SolverProcess.h"
#include <sally_api.h>

#include <memory>
#include <chrono>

const char *SolverProcess::solver = "SALLY";

using namespace sally;

struct ScopedNanoTimer
{
    std::chrono::high_resolution_clock::time_point t0;
    std::function<void(long long int)> cb;

    ScopedNanoTimer(std::function<void(long long int)> callback)
            : t0(std::chrono::high_resolution_clock::now())
            , cb(callback)
    {
    }
    ~ScopedNanoTimer(void)
    {
        auto t1 = std::chrono::high_resolution_clock::now();
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();

        cb(nanos);
    }
};

struct TimeStats{
    double total = 0;

    void operator()(long long int nanosecs) {
        total += nanosecs / (1000*1000*1000.0);
    }
};

struct ContextWrapper {
    sally_context ctx;
    SolverProcess *process;
    TimeStats ts;

    ~ContextWrapper() {
        delete_context(ctx);
    }

    ContextWrapper(SolverProcess *process) : process(process) {}
};

std::vector<net::Lemma> lemmas;

void new_lemma_eh(void *state, size_t level, const sally::expr::term_ref &lemma) {
    auto cw = static_cast<ContextWrapper *>(state);
    ScopedNanoTimer times(std::ref(cw->ts));
    auto ctx = cw->ctx;
    auto lemma_str = sally::reachability_lemma_to_command(ctx, level, lemma);
    lemmas.push_back(net::Lemma(lemma_str, 0));
//    std::cerr << "Pushing reachability lemma" << std::endl;
}

void push_lemmas(void *state) {
//    std::cerr << "Push lemma called\n";
    auto cw = static_cast<ContextWrapper *>(state);
    ScopedNanoTimer times(std::ref(cw->ts));
    cw->process->lemma_push(lemmas);
    lemmas.clear();
}

void pull_lemmas(void *state) {
//    std::cerr << "Pull lemma called\n";
    auto cw = static_cast<ContextWrapper *>(state);
    ScopedNanoTimer times(std::ref(cw->ts));
    std::vector<net::Lemma> new_lemmas;
    cw->process->lemma_pull(new_lemmas);
    for (net::Lemma &lemma : new_lemmas) {
        sally::add_lemma(cw->ctx, lemma.smtlib);
    }
}

void new_induction_lemma_eh(void * state, size_t level, const sally::expr::term_ref &lemma,
        const sally::expr::term_ref &cex, size_t cex_depth) {
    auto cw = static_cast<ContextWrapper *>(state);
    ScopedNanoTimer times(std::ref(cw->ts));
    auto ctx = cw->ctx;
    auto lemma_str = sally::induction_lemma_to_command(ctx, level, lemma, cex, cex_depth);
    lemmas.push_back(net::Lemma(lemma_str, 0));
//    std::cerr << "Pushing induction lemma" << std::endl;
//    std::cerr << lemma_str << std::endl;
}

void SolverProcess::init() {
    std::map<std::string, std::string> opts;
    opts["engine"] = "pdkind";
    opts["solver-logic"] = "QF_LRA";
    opts["solver"] = "y2o2";
    opts["yices2-mode"] = "dpllt";
//    opts["opensmt2-simplify_itp"] = "2";
    bool share_reachability_lemmas = false;
    bool share_induction_lemmas = false;
    for (auto &key:this->header.keys(net::Header::parameter)) {
        const std::string &value = this->header.get(net::Header::parameter, key);
        if (key == "share-reachability-lemmas") {
            share_reachability_lemmas = (value == "true");
            continue;
        }
        if (key == "share-induction-lemmas") {
            share_induction_lemmas = (value == "true");
            continue;
        }
        opts[key] = value;
    }
    sally_context ctx = create_context(opts);
    ContextWrapper *wrapper = new ContextWrapper(this);
    wrapper->ctx = ctx;
    this->state.reset(wrapper);

    if (share_reachability_lemmas) {
//        std::cout << "Sharing reachability lemmas" << std::endl;
        sally::set_new_reachability_lemma_eh(ctx, new_lemma_eh, wrapper);
    }
    if (share_induction_lemmas) {
//        std::cout << "Sharing induction lemmas" << std::endl;
        sally::set_obligation_pushed_eh(ctx, new_induction_lemma_eh, wrapper);
    }
    sally::add_next_frame_eh(ctx, push_lemmas, wrapper);
    sally::add_next_frame_eh(ctx, pull_lemmas, wrapper);
}

void SolverProcess::solve() {
    ContextWrapper *wrapper = static_cast<ContextWrapper * >(this->state.get());
    sally_context ctx = wrapper->ctx;
    std::string instance = this->instance + this->header["query"];
    while (true) {
        // Capture std::cout
        std::stringstream buffer;
        std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());
        try {
            if (instance.find("set-logic HORN") != std::string::npos) {
                run_on_chc_string(instance, ctx);
            }
            else {
                run_on_mcmt_string(instance, ctx);
            }

            sally::stats stats(ctx);
            auto keyvalpairs = stats.get_stats();
            for (auto& keyval : keyvalpairs) {
                this->header.set(net::Header::statistic, keyval.first, keyval.second);
            }
            this->header.set(net::Header::statistic, "lemma_sharing_time", std::to_string(wrapper->ts.total));

            // get the result
            std::string res = buffer.str();
            if (res.rfind("valid") == 0) {
                report(Status::unsat);
            } else if (res.rfind("invalid") == 0) {
                report(Status::sat);
            } else {
                error(res);
            }
            std::cout.rdbuf(old);
        } catch (std::logic_error &ex) {
            error(ex.what());
        }

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