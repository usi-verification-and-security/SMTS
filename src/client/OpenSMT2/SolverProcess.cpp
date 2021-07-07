//
// Author: Matteo Marescotti
//

#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <random>
#include <mutex>
#include "client/SolverProcess.h"
#include "OpenSMTSolver.h"


using namespace opensmt;

OpenSMTSolver *openSMTSolver = nullptr;

const char *SolverProcess::solver = "OpenSMT2";
//Color::Modifier def(Color::BG_GREEN);
//std::mutex mtx_solve;

void SolverProcess::init() {
    //mtx_solve.lock();
//    FILE *file = fopen("/dev/null", "w");
//    dup2(fileno(file), fileno(stdout));
//    dup2(fileno(file), fileno(stderr));
//    fclose(file);

    static const char *default_split = "scattering";
    static const char *default_seed = "0";

    if (this->header.get(net::Header::parameter, "seed").size() == 0) {
        this->header.set(net::Header::parameter, "seed", default_seed);
    }

    if (this->header.get(net::Header::parameter, "split").size() > 0 &&
        this->header.get(net::Header::parameter, "split") != spts_lookahead &&
        this->header.get(net::Header::parameter, "split") != spts_scatter) {
        this->warning(
                "bad parameter.split: '" + this->header.get(net::Header::parameter, "seed") + "'. using default (" +
                default_split +
                ")");
        this->header.remove(net::Header::parameter, "split");
    }
    if (this->header.get(net::Header::parameter, "split").size() == 0) {
        this->header.set(net::Header::parameter, "split", default_split);
    }
}

void SolverProcess::solve() {
    const char *msg;
    SMTConfig config;
    std::chrono::duration<long long> minSleep = std::chrono::seconds(4);
    std::chrono::time_point wakeupAt(std::chrono::system_clock::now() + minSleep);
    config.setRandomSeed(atoi(this->header.get(net::Header::parameter, "seed").c_str()));
    std::string smtlib= this->instance;
//    Logger::writeIntoFile(false,
//                          "Solver -> instance: "
//            ,(char *) (smtlib + this->header["query"]).c_str()
//            ,getpid());
    openSMTSolver = new OpenSMTSolver(this->header, config, (char *) (smtlib + this->header["query"]).c_str());
    config.SMTConfig::o_smts_check_sat_ON = true;

    std::thread t_Solve([&] {
        search();
    });
    t_Solve.detach();
    if (lemma_server)
    {
        while (true) {
            wakeupAt = std::chrono::system_clock::now() + minSleep;
            //openSMTSolver->getMainSplitter().getChannel();
            std::unique_lock<std::mutex> lk(openSMTSolver->getChannel().getMutex());
            if (openSMTSolver->getChannel().waitUntil(lk, wakeupAt)) {
                if (not openSMTSolver->getChannel().empty()) {
                    std::vector<net::Lemma> lemmas;
//Read buffer
                    std::cout << "\033[1;51m [t comunication] copying shared clauses -> size::\033[0m"
                              << openSMTSolver->getChannel().size() << std::endl;
                    for (auto term = openSMTSolver->getChannel().cbegin();
                         term != openSMTSolver->getChannel().cend(); ++term) {
                        //std::cout <<"term->first: "<< term->first<< std::endl;
                        //std::cout <<"term->level: "<< term->second<< std::endl;
                        lemmas.push_back(net::Lemma(term->first, term->second));
                    }
//Synchronize with CC
                    lemma_push(lemmas);

                    openSMTSolver->getChannel().clear();
                    openSMTSolver->getChannel().clearClauseReady();
                    lemmas.clear();
                }
            }
            lk.unlock();

            std::vector<net::Lemma> pulled_lemmas;
//Read from CC
            this->lemma_pull(pulled_lemmas);
            //sleep(1);

            if (pulled_lemmas.size()) {
                std::cout << "[t comunication] Signal to OpenSMT to stop " << "\tLemmas:"<<pulled_lemmas.size() << std::endl;
                //std::unique_lock<std::mutex> lk(openSMTSolver->getChannel().getMutex());
                lk.lock();
//Request OpenSMT to stop
                openSMTSolver->getChannel().setShouldStop();
                //lk.unlock();
//Wait to be notifed by OpenSMT
                openSMTSolver->getChannel().wait_OnPush(lk);
                //openSMTSolver->getChannel().setShallStop();
                openSMTSolver->getChannel().clearClauseReadyToPull();
//Inject read clauses
                clausesUpdate(pulled_lemmas);
                //lk.unlock();
//Notify OpenSMT to resume
                openSMTSolver->getChannel().setShallStop();
                openSMTSolver->getChannel().notify_one();
                pulled_lemmas.clear();
                lk.unlock();
            }

        }
    }
}
//void SolverProcess::pull()
//{
//    while (true) {
//        std::vector<net::Lemma> pulled_lemmas;
//
//        this->lemma_pull(pulled_lemmas);
//        //sleep(1);
//
//        if (pulled_lemmas.size()) {
//            std::unique_lock<std::mutex> lk(openSMTSolver->getChannel().getMutex());
//            std::cout << "[t comunication] Signal to OpenSMT to stop " << "\tLemmas:"<<pulled_lemmas.size() << std::endl;
//            //std::unique_lock<std::mutex> lk(openSMTSolver->getChannel().getMutex());
//            lk.lock();
////Request OpenSMT to stop
//            openSMTSolver->getChannel().setShouldStop();
//            //lk.unlock();
////Wait to be notifed by OpenSMT
//            openSMTSolver->getChannel().wait_OnPush(lk);
//            //openSMTSolver->getChannel().setShallStop();
//            openSMTSolver->getChannel().clearClauseReadyToPull();
////Inject read clauses
//            clausesUpdate(pulled_lemmas);
//            //lk.unlock();
////Notify OpenSMT to resume
//            openSMTSolver->getChannel().setShallStop();
//            openSMTSolver->getChannel().notify_one();
//            pulled_lemmas.clear();
//            lk.unlock();
//        }
//    }
//}
void SolverProcess::search()
{
    std::string smtlib;
    while (true)
    {
        openSMTSolver->getChannel().getMutex().lock();
        opensmt::stop = false;
        openSMTSolver->getChannel().getMutex().unlock();

#ifdef ENABLE_DEBUGING
        std::thread first (Logger::writeIntoFile,false,"SolverProcess: Start to Solve...","Recieved command: "+header["command"],getpid());
            first.join();
        std::cout<<"Solver Child ProcessId: "<<getpid()<<endl;
#endif
        //std::cout << "[t solve] instance -> " <<(char *) (smtlib + this->header["query"]).c_str()<<endl<<endl;
//        Logger::writeIntoFile(false,
//                "Solver -> instance: "
//                ,(char *) (smtlib + this->header["query"]).c_str()
//                ,getpid());
        openSMTSolver->interpret->interpFile((char *) (smtlib + this->header["query"]).c_str());
        openSMTSolver->getChannel().getMutex().lock();
        openSMTSolver->result = openSMTSolver->getMainSplitter().getStatus();
        opensmt::stop = false;

        if (openSMTSolver->result == s_True) {
            this->report(Status::sat);
        }
        else if (openSMTSolver->result == s_False) {

            this->report(Status::unsat);
        }
        Task task = this->wait();
        switch (task.command) {
            case Task::incremental:
                smtlib = task.smtlib;
                if (openSMTSolver->learned_push) {
                    openSMTSolver->learned_push = false;
                    std::cout << "\033[1;51m [t solve] pop \033[0m"<<endl;
                    //openSMTSolver->getMainSplitter().pop();

                    break;
                }
            case Task::resume:
                smtlib.clear();
                break;

        }
        openSMTSolver->getChannel().getMutex().unlock();
#ifdef ENABLE_DEBUGING
        std::thread log (Logger::writeIntoFile,false,"SolverProcess: Finished solving...","Recieved command: "+header["command"],getpid());
            log.join();
#endif
    }
}

void SolverProcess::interrupt() {

    std::scoped_lock<std::mutex> lk(openSMTSolver->getChannel().getMutex());
    opensmt::stop= true;
}

void SolverProcess::partition(uint8_t n) {
    pid_t pid = getpid();
    //fork() returns -1 if it fails, and if it succeeds, it returns the forked child's pid in the parent, and 0 in the child.
    // So if (fork() != 0) tests whether it's the parent process.
#ifdef ENABLE_DEBUGING
    std::thread log (Logger::writeIntoFile,false,"SolverProcess - Main Thread: Start to fork partition process",
                     "Recieved command: "+header["command"],getpid());
        log.join();
#endif
    if (fork() != 0) {
        return;
    }
    std::thread _t([&] {
        while (getppid() == pid)
            sleep(1);
        exit(0);
    });
    //FILE *file = fopen("/dev/null", "w");
    //dup2(fileno(file), fileno(stdout));
    //dup2(fileno(file), fileno(stderr));
    //fclose(file);
#ifdef ENABLE_DEBUGING
    std::thread logger (Logger::writeIntoFile,false,"PartitionProcess - Main Thread: Start to set SMTConfig to do partiotion",
                     "Recieved command: "+header["command"],getpid());
        logger.join();
#endif
    std::vector<std::string> partitions;
    const char *msg;
    if (!(
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_num,
                                                                   SMTOption(int(n)),
                                                                   msg) &&
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_type,
                                                                   SMTOption(this->header.get(net::Header::parameter, "split")
                                                                                     .c_str()),
                                                                   msg) &&
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_smt_split_format_length,
                                                                   SMTOption("brief"),
                                                                   msg) &&
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_units, SMTOption(spts_time), msg) &&
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_inittune, SMTOption(double(2)), msg) &&
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_midtune, SMTOption(double(2)), msg) &&
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_asap, SMTOption(1), msg))) {
        this->report(partitions, msg);
    }
    else {
#ifdef ENABLE_DEBUGING
        std::thread log (Logger::writeIntoFile,false,"PartitionProcess - Main Thread: Start to partitioning",
                     "Recieved command: "+header["command"],getpid());
        log.join();
#endif
        sstat status = openSMTSolver->getMainSplitter().solve();
        if (status == s_Undef) {
            partitions = openSMTSolver->getMainSplitter().getSolverPartitions();
            std::cout <<"Recieved PN from SMTS Server-> "<<int(n)<<"\tSplits constructed by solver: "<<partitions.size()<<endl;
            this->report(partitions);
        } else if (status == s_True) {
            this->report(Status::sat);
        }
        else if (status == s_False) {
            this->report(Status::unsat);
        }
        else {
            this->report(partitions, "error during partitioning");
        }
    }
    partitions.clear();
    exit(0);
}

void SolverProcess::getCnfClauses(net::Header &header, const std::string &payload) {
    if (header.count("query")) {

        pid_t pid = getpid();
        if (fork() != 0) {
            return;
        }

        std::thread _t([&] {
            while (getppid() == pid) {
                sleep(1);
            }
            exit(0);
        });

        SMTConfig config;
        config.set_dryrun(true);
        openSMTSolver = new OpenSMTSolver(header, config, nullptr);
        openSMTSolver->interpret->interpFile((char *) (payload + header["query"]).c_str());

        char *cnf = openSMTSolver->getMainSplitter().getSMTSolver().printCnfClauses();
        this->report(header, header["command"], cnf);
        free(cnf);
        exit(0);
    } else {
        char *cnf = openSMTSolver->getMainSplitter().getSMTSolver().printCnfClauses();
        this->report(header, header["command"], cnf);
        free(cnf);
    }
}

void SolverProcess::getCnfLearnts(net::Header &header) {
    char *cnf = openSMTSolver->getMainSplitter().getSMTSolver().printCnfLearnts();
    this->report(header, header["command"], cnf);
    free(cnf);
}
void SolverProcess::clausesPublish(net::Lemma& lemma ) {
    std::vector<net::Lemma> lemmas;
#ifdef ENABLE_DEBUGING
    Logger::writeIntoFile,false,"clausesPublish: openSMTSolver->lemma_push Not Null ",to_string(static_cast<ScatterSplitter&>(openSMTSolver->getMainSplitter().getSMTSolver()).learnts_size),getpid();
#endif
    lemmas.push_back(net::Lemma(lemma));
    lemma_push(lemmas);
}
void inline SolverProcess::clausesUpdate(std::vector<net::Lemma>& lemmas) {
    if (openSMTSolver->learned_push)
        openSMTSolver->getMainSplitter().pop();

    openSMTSolver->getMainSplitter().push();
    openSMTSolver->learned_push = true;
    cout << "\033[1;31m [t comunication] pull and inject some clauses -> Size::\033[0m"<< lemmas.size()<<std::endl;
    for (auto &lemma:lemmas) {
        if (lemma.smtlib.size() > 0) {
#ifdef ENABLE_DEBUGING
            std::cout <<"term->first: "<< (char *) ("(assert " + lemma.smtlib + ")").c_str()<< std::endl;
            Logger::writeIntoFile(false,"clausesUpdate: Instance ",("(assert " + lemma.smtlib + ")"),getpid());
#endif
//            Logger::writeIntoFile(false,
//                    "clausesUpdate -> instance: "
//                    ,(char *) ("(assert " + lemma.smtlib + ")").c_str()
//                    ,getpid());
            openSMTSolver->interpret->interpFile((char *) ("(assert " + lemma.smtlib + ")").c_str());

        }
    }
}