//
// Author: Matteo Marescotti
//

#ifndef SMTS_CLIENT_SOLVERPROCESS_H
#define SMTS_CLIENT_SOLVERPROCESS_H

#include <atomic>
#include <random>
#include <mutex>
#include <ctime>
#include <chrono>
#include "lib/lib.h"


enum Status {
    unknown, sat, unsat
};

struct Task {
    const enum {
        incremental, resume
    } command;
    std::string smtlib;
};


class SolverProcess : public Process {
private:
    struct {
        const uint8_t errors_max = 3;
        std::mutex mtx;
        std::shared_ptr<net::Socket> server;
        std::vector<net::Lemma> to_push;
        uint8_t errors = 0;
        uint8_t interval = 2;
        std::time_t last_push = 0;
        std::time_t last_pull = 0;
    } lemma;

    net::Pipe pipe;
    std::string instance;
    net::Header header;
    std::shared_ptr<void> state;

    void main() {
        if (!this->header.count("lemmas")) {
            this->header["lemmas"] = std::to_string(1000);
        }
        if (!this->header.count("max_memory")) {
            this->header["max_memory"] = std::to_string(0);
        }
        this->init();
        this->info("start");
        this->solve();
    }

    // here the module can edit class fields
    void init();

    // class field are read only
    void solve();


    void partition(uint8_t);

    // async interrupt the solver
    void interrupt();

    // Get CNF corresponding to a particular solver
    void getCnfClauses(net::Header &header, const std::string &payload);

    void getCnfLearnts(net::Header &header);

    void report(net::Header &header, const std::string &report, const std::string &payload) {
        if (&header != &(this->header)) {
            if (report.size())
                header["report"] = report;
            header.insert(this->header.begin(), this->header.end());
        }
        this->writer()->write(header, payload);
    }

    void report() {
        this->report(this->header, "", "");
    }

    void report(Status status, net::Header header = net::Header()) {
        if (header.size() == 0)
            header = this->header.copy(this->header.keys(net::Header::statistic));
        if (status == Status::sat)
            this->report(header, "sat", "");
        else if (status == Status::unsat)
            this->report(header, "unsat", "");
        else
            this->report(header, "unknown", "");

    }

    void report(const std::vector<std::string> &partitions, const char *error = nullptr) {
        net::Header header;
        if (error != nullptr)
            return this->error(error);
        this->report(header, "partitions", ::to_string(partitions));
    }

    void info(const std::string &info, net::Header header = net::Header()) {
        this->report(header, "info:" + info, "");
    }

    void warning(const std::string &warning, net::Header header = net::Header()) {
        this->report(header, "warning:" + warning, "");
    }

    void error(const std::string &error, net::Header header = net::Header()) {
        this->report(header, "error:" + error, "");
    }

    Task wait() {
#ifdef ENABLE_DEBUGING
        std::thread log (Logger::writeIntoFile,false,"SolverProcess - Main Thread: Start to read the pipe...","Recieved command: "+header["command"],getpid());
        log.join();
#endif
        net::Header header;
        std::string payload;
        this->pipe.reader()->read(header, payload);
        if (header["name"] != this->header["name"] || header["node"] != this->header["node"]) {
            this->error("not expected: " + header["name"] + header["node"]);
            return Task{
                    .command=Task::resume
            };
        }
        if (header["command"] == "incremental") {
            if (header.count("node_") && header.count("query")) {
                this->instance += payload;
                this->header["node"] = header["node_"];
                this->header["query"] = header["query"];
                this->info("incremental solving step from " + header["node"]);
                return Task{
                        .command=Task::incremental,
                        .smtlib=payload
                };
            }
        }
        if (header["command"] == "partition" && header.count("partitions") == 1) {
            this->partition((uint8_t) atoi(header["partitions"].c_str()));
        }
        if (header["command"] == "cnf-clauses") {
            this->getCnfClauses(header, payload);
        }
        if (header["command"] == "cnf-learnts") {
            this->getCnfLearnts(header);
        }
        return Task{
                .command=Task::resume
        };
    }

public:
    SolverProcess(net::Header header,
                  std::string instance) :
            instance(instance),
            header(header) {
        if (!header.count("name") || !header.count("node"))
            throw Exception(__FILE__, __LINE__, "missing mandatory key in header");
        this->start();
    }

    bool is_sharing() {
        return this->lemma.server != nullptr;
    }

    void lemma_push(const std::vector<net::Lemma> &lemmas) {
        if (lemmas.size() == 0 && this->lemma.to_push.size() == 0)
            return;
        std::lock_guard<std::mutex> _l(this->lemma.mtx);

        this->lemma.to_push.insert(this->lemma.to_push.end(), lemmas.begin(), lemmas.end());

        if (
                !is_sharing() ||
                std::time(nullptr) < this->lemma.last_push + this->lemma.interval ||
                this->lemma.errors >= this->lemma.errors_max
                )
            return;

        net::Header header = this->header.copy({"name", "node"});

        header["lemmas"] = "+" + std::to_string(this->lemma.to_push.size());

        try {
            this->lemma.server->write(header, ::to_string(this->lemma.to_push));
        } catch (net::SocketException &ex) {
            this->lemma.errors++;
            this->error(std::string("lemma push failed: ") + ex.what());
            return;
        }

        this->lemma.errors = 0;
        this->lemma.last_push = std::time(nullptr);
        if (this->lemma.interval > 1)
            this->lemma.interval--;

        this->lemma.to_push.clear();
    }

    void lemma_pull(std::vector<net::Lemma> &lemmas) {
        if (
                !is_sharing() ||
                std::time(nullptr) < this->lemma.last_pull + this->lemma.interval ||
                this->lemma.errors >= this->lemma.errors_max
                )
            return;

        std::lock_guard<std::mutex> _l(this->lemma.mtx);

        net::Header header = this->header.copy({"name", "node"});
        header["lemmas"] = "-" + this->header["lemmas"];
        std::string payload;

        try {
            this->lemma.server->write(header, "");
            this->lemma.server->read(header, payload);
        } catch (net::SocketException &ex) {
            this->lemma.errors++;
            this->error(std::string("lemma pull failed: ") + ex.what());
            return;
        }

        this->lemma.errors = 0;
        this->lemma.last_pull = std::time(nullptr);
        if (this->lemma.interval > 1)
            this->lemma.interval--;

        if (header["name"] != this->header["name"]
            || header["node"] != this->header["node"])
            return;

        std::istringstream is(payload);
        is >> lemmas;
    }

    static const char *solver;
};

#endif
