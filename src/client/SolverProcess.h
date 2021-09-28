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
#include "deque"
#include "../../build/_deps/opensmt-src/src/api/Channel.h"
enum Status {
    unknown, sat, unsat
};
//static const size_t Capacity = 4;
//std::string Command[Capacity] = { "incremental","partition","cnf-clauses","cnf-learnts"};

struct Task {
    const enum {
        incremental, resume, partition
    } command;
    std::string smtlib;
};


class SolverProcess : public Thread {
    friend class SolverServer;
private:
    std::vector<net::Lemma> pulled_lemmas;
    Channel channel;

    std::condition_variable cv;
    std::string instance;
    bool lemma_server;
    std::deque<std::string> instance_Temp;
    net::Header header;
    std::deque<net::Header> header_Temp;
    std::mutex mtx_listener_solve;
    int lemmaPush_timeout_min;
    int lemmaPush_timeout_max;
    int lemmaPull_timeout_min;
    int lemmaPull_timeout_max;
    std::string currentLemmaPulledNodePath;
    // async interrupt the solver
    void interrupt(string command);


    void main() override {
//        try {
            getChannel().getMutex().lock();
            this->header["lemmas"] = this->header["lemma_amount"];
            if (!this->header.count("max_memory")) {
                this->header["max_memory"] = std::to_string(0);
            }
            this->init();
            this->info("start");

            this->solve();
//        }
//        catch (...) {
//            std::cout << "exception main " << std::endl;
//        }
    }

    // here the module can edit class fields
    void init();

    // class field are read only
    void solve();

    void search();

    void partition(uint8_t);


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
        //write(header, payload);
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
        std::cout<<"start to report partition "<<endl<<"header: "<<header<<endl;
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
    Channel& getChannel()  {
        //assert(mainSolver->getChannel());
        return channel;
//       return ( (MainSplitter&) interpret->getMainSolver() ).getChannel();
    };
    Task wait(int index) {
#ifdef ENABLE_DEBUGING
        std::thread log (Logger::writeIntoFile,false,"SolverProcess - Main Thread: Start to read the pipe...","Recieved command: "+header["command"],getpid());
        log.join();
#endif
//        if (tryWait)
//        std::scoped_lock<std::mutex> _l(this->mtx_listener_solve);
        if (header_Temp[index]["name"] != this->header_Temp[index]["name"] || header_Temp[index]["node"] != this->header_Temp[index]["node"]) {
            this->error("not expected: " + header_Temp[index]["name"] + header_Temp[index]["node"]);
            return Task{
                    .command=Task::resume
            };
        }
//        if (header_Temp["command"] == "stop") {
//            return Task{
//                    .command=Task::stop,
//            };
//        }
        if (header_Temp[index]["command"] == "incremental") {
            if (header_Temp[index].count("node_") && header_Temp[index].count("query")) {
                this->instance += instance_Temp[index];
//                string temp =instance_Temp[index];
                this->header["node"] = header_Temp[index]["node_"];
                this->header["query"] = header_Temp[index]["query"];
                this->info("incremental solving step from " + header_Temp[index]["node"]);
//                header_Temp.erase(header_Temp.begin()+index,header_Temp.begin() + index + 1);
//                instance_Temp.erase(instance_Temp.begin()+index,instance_Temp.begin() + index + 1);
                return Task{
                        .command =Task::incremental,
                        .smtlib =instance_Temp[index]
                };
            }
        }
        if (header_Temp[index]["command"] == "partition" && header_Temp[index].count("partitions") == 1) {
            this->partition((uint8_t) atoi(header_Temp[index]["partitions"].c_str()));
//            return Task{
//                    .command=Task::partition,
//            };
        }
        if (header_Temp[index]["command"] == "cnf-clauses") {
            this->getCnfClauses(header_Temp[index], instance_Temp[index]);
        }
        if (header_Temp[index]["command"] == "cnf-learnts") {
            this->getCnfLearnts(header_Temp[index]);
        }
//        header_Temp.erase(header_Temp.begin()+index,header_Temp.begin() + index + 1);
//        instance_Temp.erase(instance_Temp.begin()+index,instance_Temp.begin() + index + 1);
        return Task{
                .command=Task::resume
        };
    }

public:
    struct {
        const uint8_t errors_max = 3;
        std::mutex mtx_li;
        std::shared_ptr<net::Socket> server;
//        std::vector<net::Lemma> to_push;
        uint8_t errors = 0;
        uint8_t interval = 3;
        std::time_t last_push = 0;
        std::time_t last_pull = 0;
    } lemma;

    SolverProcess(net::Header header,
                  std::string instance, net::Socket* server, bool lemma_server) :
                  instance(instance),
                  lemma_server(lemma_server),
                  header(header)
                  {
                      smtsServer = server;
                      if (!header.count("name") || !header.count("node"))
                          throw Exception(__FILE__, __LINE__, "missing mandatory key in header");
                      start_thread("solver");
                  }

    ~SolverProcess(){if(pulled_lemmas.empty()) pulled_lemmas.clear();}
    bool is_sharing() {
        return this->lemma.server != nullptr;
    }

    void lemma_push(const std::vector<net::Lemma> &lemmas) {
        if (lemmas.empty()) return;
        std::unique_lock<std::mutex> _l(this->mtx_listener_solve);
//        std::scoped_lock<std::mutex> _loc(this->lemma.mtx_li);
        net::Header header = this->header.copy({"name", "node"});
        header["lemmas"] = "+" + std::to_string(lemmas.size());

        try {
            std::cout << "[t push -> PID= "+to_string(getpid())+" ] SWriting lemmas to LemmaServer: -> size::"<< lemmas.size()<< std::endl;
            _l.unlock();
            this->lemma.server->write(header, ::to_string(lemmas));
            std::cout << "[t push -> PID= "+to_string(getpid())+" ] EWriting lemmas to LemmaServer: -> size::"<< lemmas.size()<< std::endl;
        } catch (net::SocketException &ex) {
            _l.unlock();
            this->lemma.errors++;
            this->error(std::string("lemma push failed: ") + ex.what());
            return;
        }

        this->lemma.errors = 0;
        this->lemma.last_push = std::time(nullptr);
        if (this->lemma.interval > 1)
            this->lemma.interval--;
    }

    bool lemma_pull() {
        std::vector<net::Lemma> lemmas;
        std::unique_lock<std::mutex> _l(this->mtx_listener_solve);
//        if (
//                !is_sharing() ||
//                std::time(nullptr) < this->lemma.last_pull + this->lemma.interval ||
//                this->lemma.errors >= this->lemma.errors_max
//                )
//            return false;

//        this->mtx_listener_solve.lock();
        net::Header header = this->header.copy({"name", "node"});
        header["lemmas"] = "-" + this->header["lemmas"];
        std::string payload;

        try {
            std::cout << "[t push -> PID= "+to_string(getpid())+" ] SReading lemmas from LemmaServer from node -> "+header["node"]<< std::endl;
            _l.unlock();
            this->lemma.server->write(header, "");
            this->lemma.server->read(header, payload);
            _l.lock();
        } catch (net::SocketException &ex) {
            this->lemma.errors++;
            this->error(std::string("lemma pull failed: ") + ex.what());
            return false;
        }

        this->lemma.errors = 0;
        this->lemma.last_pull = std::time(nullptr);
        if (this->lemma.interval > 1)
            this->lemma.interval--;

        if ( payload.empty() or header["name"] != this->header["name"]) return false;
        currentLemmaPulledNodePath = header["node"];
        std::istringstream is(payload);
        is >> lemmas;
        std::cout << "[t push -> PID= "+to_string(getpid())+" ] EReading lemmas from LemmaServer: -> size::"<< lemmas.size()<< std::endl;
        pulled_lemmas.insert(std::end(pulled_lemmas),
                             std::begin(lemmas), std::end(lemmas));
        _l.unlock();
//        cout << "\033[1;31m [t pull] pull and accumulate clauses -> Size::\033[0m" << lemmas.size()<< std::endl;
        return true;
    }
    inline bool isPrefix(std::string_view prefix, std::string_view full)
    {
        return prefix == full.substr(0, prefix.size());
    }
    static const char *solver;
    void injectPulledClauses();
    void clausePush() override;
    void clausePull() override;
    void checkForlearned_pushBeforIncrementality();
};

#endif
