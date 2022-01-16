//
// Author: Matteo Marescotti
//

#ifndef SMTS_CLIENT_SOLVERPROCESS_H
#define SMTS_CLIENT_SOLVERPROCESS_H

#include <random>
#include <mutex>
#include <ctime>
#include <chrono>
#include "deque"
#include "../../build/_deps/opensmt-src/src/api/Channel.h"
#include "../../build/_deps/opensmt-src/src/common/Printer.h"
#include "lib/lib.h"


class SolverProcess  : public SMTSThread {
    friend class SolverServer;
private:
    opensmt::synced_stream synced_stream;
    opensmt::StoppableWatch timer;
    net::Header header;
//    net::Socket* SMTSServer;
//    std::vector<std::thread> vecOfThreads;
    std::map<std::string , std::vector<net::Lemma>>  node_PulledLemmas;
    Channel & channel;
    std::string instance;
    std::deque<std::string> instance_Temp;
//    bool colorMode;
    std::deque<net::Header> header_Temp;
    mutable std::mutex mtx_listener_solve;
    pid_t child_pid;

    // async interrupt the solver
    void interrupt(const string& command);
    void kill_child(int sig);

    void main()  {
//        try {
        getChannel().getMutex().lock();
        if (!this->header.count("lemmas")) {
            this->header["lemmas"] = std::to_string(1000);
        }

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
    }

    void report() {
        this->report(this->header, "", "");
    }

    void report(PartitionChannel::Status status, const std::string& status_inf, const std::string& conflict="" , net::Header header = net::Header()) {
        if (header.size() == 0)
            header = this->header.copy(this->header.keys(net::Header::statistic));
        header["conflict"] = conflict;
        header["statusinf"] = status_inf;
        if (status == PartitionChannel::Status::sat)
            this->report(header, "sat", "");
        else if (status == PartitionChannel::Status::unsat)
            this->report(header, "unsat", "");
        else
            this->report(header, "unknown", "");

    }

    void report(const std::vector<std::string> &partitions, const std::string& conflict, const std::string& status, const char *error = nullptr) {
        net::Header header;
        header["conflict"] = conflict;
        header["statusinf"] = status;
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
    inline Channel& getChannel()  {  return channel; };

    PartitionChannel::Task wait(int index) {

//        if (tryWait)
//        std::scoped_lock<std::mutex> _l(this->mtx_listener_solve);
        if (header_Temp[index]["name"] != this->header_Temp[index]["name"] || header_Temp[index]["node"] != this->header_Temp[index]["node"]) {
            this->error("not expected: " + header_Temp[index]["name"] + header_Temp[index]["node"]);
            return PartitionChannel::Task{
                    .command = PartitionChannel::Task::resume
            };
        }
//        if (header_Temp["command"] == "stop") {
//            return Task{
//                    .command=Task::stop,
//            };
//        }
        if (header_Temp[index]["command"] == PartitionChannel::Command.Incremental) {
            if (header_Temp[index].count("node_") && header_Temp[index].count("query")) {
                this->instance += instance_Temp[index];
//                string temp =instance_Temp[index];
                this->header["node"] = header_Temp[index]["node_"];
                this->header["query"] = header_Temp[index]["query"];
                if (this->header["enableLog"] == "1")
                    this->info("incremental solving step from " + header_Temp[index]["node"]);
                return PartitionChannel::Task {
                        .command = PartitionChannel::Task::incremental,
                        .smtlib = instance_Temp[index]
                };
            }
        }
        if (header_Temp[index]["command"] == PartitionChannel::Command.Partition && header_Temp[index].count("partitions") == 1) {

            this->partition((uint8_t) atoi(header_Temp[index]["partitions"].c_str()));
//            return Task{
//                    .command=Task::partition,
//            };
        }
        if (header_Temp[index]["command"] == PartitionChannel::Command.CnfClauses) {
            this->getCnfClauses(header_Temp[index], instance_Temp[index]);
        }
        if (header_Temp[index]["command"] == PartitionChannel::Command.Cnflearnts) {
            this->getCnfLearnts(header_Temp[index]);
        }
        return PartitionChannel::Task{
                .command=PartitionChannel::Task::resume
        };
    }

public:
    struct {
        const uint8_t errors_max = 3;
        std::shared_ptr<net::Socket> server;
//        std::vector<net::Lemma> to_push;
        uint8_t errors = 0;
        uint8_t interval = 3;
        std::time_t last_push = 0;
        std::time_t last_pull = 0;
        mutable std::mutex lemma_mutex;
    } lemma;

    SolverProcess(net::Header header,std::string instance, net::Socket* server,Channel& ch, bool lemma_server) :
            synced_stream(std::clog),
            header (header),
            channel(ch),
            instance (instance)
    {
        SMTSServer = server;
        if (!header.count("name") || !header.count("node"))
            throw Exception(__FILE__, __LINE__, "missing mandatory key in header");

        start_Thread(PartitionChannel::ThreadName::Comunication);
        start_Thread(PartitionChannel::ThreadName::MemCheck);
        if (lemma_server)
            start_lemma_threads();
    }
    void memoryCheck()
    {
        size_t limit = atoll(header["max_memory"].c_str());
        if (limit == 0)
            return;

        while (true) {
            size_t cmem = current_memory();
            if (cmem > limit * 1024 * 1024) {
                this->error(std::string("max memory reached: ") + std::to_string(cmem));
//                exit(-1);
            }
            std::unique_lock<std::mutex> lk(channel.getMutex());
            if (channel.waitFor(lk, std::chrono::seconds (3)))
                break;
        }
    }
    void start_lemma_threads()
    {
        std::string seed = this->header.get(net::Header::parameter, "seed");
        int interval= atoi(this->header["lemma_push_min"].c_str()) + ( atoi(seed.substr(1, seed.size() - 1).c_str())
                                                  % ( atoi(this->header["lemma_push_max"].c_str())- atoi(this->header["lemma_push_min"].c_str()) + 1 ) );
        getChannel().setClauseShareMode();

        getChannel().setClauseLearnInterval(interval/2);

        start_Thread(PartitionChannel::ThreadName::ClausePush, seed,this->header["lemma_push_min"],this->header["lemma_push_max"]);

        start_Thread(PartitionChannel::ThreadName::ClausePull,seed,this->header["lemma_pull_min"], this->header["lemma_pull_max"]);

        this->header.erase("lemma_push_min");
        this->header.erase("lemma_push_max");
        this->header.erase("lemma_pull_max");
        this->header.erase("lemma_pull_min");
        this->header.erase("colorMode");
    }
    ~SolverProcess()
    {
        this->wait_ForThreads();
    }

    bool is_sharing() {
        return this->lemma.server != nullptr;
    }

    void lemma_push(const std::map<std::string, std::vector<net::Lemma>> &lemmas) {
        if (lemmas.empty()) return;
        for ( const auto &toPush_lemma : lemmas )
        {
            std::unique_lock<std::mutex> _l(this->mtx_listener_solve);
            if (!is_sharing())
                return;
            net::Header header = this->header.copy({"name", "node"});
            header["node"] = toPush_lemma.first;
            header["lemmas"] = "+" + std::to_string(toPush_lemma.second.size());

            try {
#ifdef ENABLE_DEBUGING
                std::cout << "[t push ]-> PID= "+to_string(getpid())+" ] SWriting lemmas to LemmaServer: -> size::"<< toPush_lemma.second.size()
                          <<"   from node -> "+header["node"]<< std::endl;
#endif
                _l.unlock();
                this->lemma.lemma_mutex.lock();
                this->lemma.server->write(header, ::to_string(toPush_lemma.second));
//            std::cout<<::to_string(lemmas);
//            exit(0);
                this->lemma.lemma_mutex.unlock();
#ifdef ENABLE_DEBUGING
                std::cout << "[t push ]-> PID= "+to_string(getpid())+" ] EWriting lemmas to LemmaServer: -> size::"<< toPush_lemma.second.size()<< std::endl;
#endif
            } catch (net::SocketException &ex) {
                this->lemma.lemma_mutex.unlock();
                this->lemma.errors++;
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                this->error(std::string("lemma push failed: ") + ex.what());
                return;
            }
        }
    }

    bool lemma_pull() {
        int lemma_Size = 0;
        std::vector<net::Lemma> lemmas;
        std::unique_lock<std::mutex> _l(this->mtx_listener_solve);
        if (!is_sharing())
            return false;

        net::Header header = this->header.copy({"name", "node"});
        header["lemmas"] = "-" + this->header["lemmas"];
        std::string payload;

        try {
#ifdef ENABLE_DEBUGING
            std::cout << "[t pull ]-> PID= "+to_string(getpid())+" ] SReading lemmas from LemmaServer for node -> "+header["node"]<< std::endl;
#endif
            _l.unlock();
            this->lemma.lemma_mutex.lock();
            this->lemma.server->write(header, "");
            this->lemma.server->read(header, payload);
            this->lemma.lemma_mutex.unlock();
            _l.lock();
        } catch (net::SocketException &ex) {
            this->lemma.lemma_mutex.unlock();
            this->lemma.errors++;
            this->error(std::string("lemma pull failed: ") + ex.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            return false;
        }

        this->lemma.errors = 0;
        this->lemma.last_pull = std::time(nullptr);
        if (this->lemma.interval > 1)
            this->lemma.interval--;

        if (payload.empty() or header["name"] != this->header["name"] ) return node_PulledLemmas.size();
//        currentLemmaPulledNodePath = header["node"];
        std::istringstream is(payload);
        is >> lemmas;
#ifdef ENABLE_DEBUGING
        std::cout << "[t pull -> PID= "+to_string(getpid())+" ] EReading lemmas from LemmaServer: -> size::"<< lemmas.size()<< std::endl;
#endif
        node_PulledLemmas[header["node"]].insert(std::end(node_PulledLemmas[header["node"]]),
                                                 std::begin(lemmas), std::end(lemmas));
        lemma_Size = node_PulledLemmas.size();
        _l.unlock();
        return lemma_Size;
    }
    inline bool isPrefix(std::string_view prefix, std::string_view full)
    {
        return prefix == full.substr(0, prefix.size());
    }
    static const char *solver;
    void injectPulledClauses(const std::string & nodePath);
    void clausePush(const string & seed, const string & n1, const string & n2) ;
    void clausePull(const string & seed, const string & n1, const string & n2) ;
    void checkForlearned_pushBeforIncrementality();

};

#endif
