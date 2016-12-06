//
// Created by Matteo on 21/07/16.
//

#ifndef CLAUSE_SHARING_PROCESSSOLVER_H
#define CLAUSE_SHARING_PROCESSSOLVER_H

#include <atomic>
#include <mutex>
#include "lib/lib.h"


enum Status {
    unknown, sat, unsat
};

struct Task {
    const enum {
        incremental, resume
    } command;
    std::string smtlib;
    uint8_t partitions;
};


class SolverProcess : public Process {
private:
    std::vector<net::Lemma> lemma_to_push;
    std::shared_ptr<net::Socket> lemmas;
    std::mutex lemmas_mtx;
    static const uint8_t lemmas_errors_max = 3;
    uint8_t lemmas_errors;

    net::Pipe pipe;
    std::string instance;

    void main() {
        if (!this->header.count("lemmas")) {
            this->header["lemmas"] = std::to_string(1000);
        }
        this->init();
        auto header = this->header;
        header["info"] = "start solving";
        this->report(header);
        std::thread t([&] {
            net::Header header;
            std::string payload;
            while (true) {
                // receives both commands from the server(forwarded by SolverServer) and from SolverServer.
                // if command=local the answer is built here and delivered to SolverServer
                // otherwise the solver is interrupted and the message forwarder through pipe
                this->reader()->read(header, payload);
                if (!header.count("command"))
                    continue;
                if (header["command"] == "local" && header.count("local")) {
                    if (header["local"] == "report")
                        this->report();
                    else if (header["local"] == "lemma_server" && header.count("lemma_server")) {
                        if (!header["lemma_server"].size()) {
                            std::lock_guard<std::mutex> lock(this->lemmas_mtx);
                            this->lemmas.reset();
                        } else {
                            try {
                                std::lock_guard<std::mutex> lock(this->lemmas_mtx);
                                this->lemmas.reset(new net::Socket(header["lemma_server"]));
                                this->lemmas_errors = 0;
                            } catch (net::SocketException &ex) {
                                this->error(std::string("lemma server connection failed: ") + ex.what());
                                continue;
                            }
                            this->lemma_push(std::vector<net::Lemma>());
                        }
                    }
                    continue;
                }
                this->interrupt();
                this->pipe.writer()->write(header, payload);
            }
        });
        this->solve();
    }

    // here the module can edit class fields
    void init();

    // class field are read only
    void solve();


    void partition(uint8_t);

    // async interrupt the solver
    void interrupt();

    void report(net::Header &header, const std::string &payload = "") {
        if (!header.count("name"))
            header["name"] = this->header["name"];
        if (!header.count("node"))
            header["node"] = this->header["node"];
        this->writer()->write(header, payload);
    }

    void report() {
        this->writer()->write(this->header, "");
    }

    void
    report(Status status, const net::Header &h = net::Header()) {
        auto header = h;
        if (status == Status::sat)
            header["status"] = "sat";
        else if (status == Status::unsat)
            header["status"] = "unsat";
        else
            header["status"] = "unknown";
        this->report(header);

    }

    void report(const std::vector<std::string> &partitions, const char *error = nullptr) {
        net::Header header;
        std::stringstream payload;
        if (error != nullptr)
            header["error"] = error;
        ::join(payload, "\n", partitions);
        header["partitions"] = std::to_string(partitions.size());
        this->report(header, payload.str());
    }

    void info(const std::string &info) {
        net::Header header;
        header["info"] = info;
        this->report(header);
    }

    void warning(const std::string &warning) {
        net::Header header;
        header["warning"] = warning;
        this->report(header);
    }

    void error(const std::string &error) {
        net::Header header;
        header["error"] = error;
        this->report(header);
    }

    Task wait() {
        net::Header header;
        std::string payload;
        this->pipe.reader()->read(header, payload);
        if (header["command"] == "incremental") {
            return Task{
                    .command=Task::incremental,
                    .smtlib=payload
            };
        }
        if (header["command"] == "partition" && header.count("partitions") == 1) {
            this->partition((uint8_t) atoi(header["partitions"].c_str()));
        }
        return Task{
                .command=Task::resume
        };
    }

    void lemma_push(const std::vector<net::Lemma> &lemmas) {
        if (lemmas.size() == 0 && this->lemma_to_push.size() == 0)
            return;

        std::lock_guard<std::mutex> _l(this->lemmas_mtx);

        this->lemma_to_push.insert(this->lemma_to_push.end(), lemmas.begin(), lemmas.end());

        if (!is_sharing() || this->lemmas_errors >= SolverProcess::lemmas_errors_max)
            return;

        net::Header header = this->header;
        header["name"] = this->header["name"];
        header["node"] = this->header["node"];
        std::stringstream payload;

        payload << this->lemma_to_push;

        header["separator"] = "a";
        header["lemmas"] = std::to_string(this->lemma_to_push.size());

        try {
            this->lemmas->write(header, payload.str());
        } catch (net::SocketException &ex) {
            this->lemmas_errors++;
            this->error(std::string("lemma push failed: ") + ex.what());
            return;
        }
        this->lemma_to_push.clear();
    }

    void lemma_pull(std::vector<net::Lemma> &lemmas) {
        if (!is_sharing() || this->lemmas_errors >= SolverProcess::lemmas_errors_max)
            return;

        std::lock_guard<std::mutex> _l(this->lemmas_mtx);

        net::Header header;
        header["name"] = this->header["name"];
        header["node"] = this->header["node"];
        header["lemmas"] = this->header["lemmas"];
        std::string payload;

        try {
            this->lemmas->write(header, "");
            this->lemmas->read(header, payload, 2000);
        } catch (net::SocketException &ex) {
            this->lemmas_errors++;
            this->error(std::string("lemma pull failed: ") + ex.what());
            return;
        } catch (net::SocketTimeout &) {
            this->lemmas_errors++;
            this->error("lemma pull failed: timeout");
            return;
        }

        if (header["name"] != this->header["name"]
            || header["node"] != this->header["node"]
            || header.count("separator") == 0)
            return;

        std::istringstream is(payload);
        is >> lemmas;
    }

public:
    SolverProcess(net::Header header,
                  std::string instance) :
            lemmas_errors(0),
            instance(instance),
            header(header) {
        this->start();
    }

    net::Header header;

    bool is_sharing() {
        return this->lemmas != nullptr;
    }

    static const char *solver;
};

#endif //CLAUSE_SHARING_PROCESSSOLVER_H
