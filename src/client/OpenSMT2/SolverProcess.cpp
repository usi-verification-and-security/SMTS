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

std::unique_ptr<OpenSMTSolver> openSMTSolver;
const char *SolverProcess::solver = "OpenSMT2";


void SolverProcess::init() {
    //mtx_solve.lock();
//    FILE *file = fopen("/dev/null", "w");
//    dup2(fileno(file), fileno(stdout));
//    dup2(fileno(file), fileno(stderr));
//    fclose(file);

    static const char *default_split = spts_scatter;
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

    SMTConfig config;

    config.setRandomSeed(atoi(this->header.get(net::Header::parameter, "seed").c_str()));

    openSMTSolver.reset( new OpenSMTSolver(config,  this->instance, getChannel()));

    search();
}

void SolverProcess::clausePull(const string & seed, const string & n1, const string & n2)
{
    try {
//        time_t start;
//        time_t finish;
        int counter = 0;
//        srand(getpid() + time(NULL));
        //channel.getMutex().lock();
        int interval = atoi(n1.c_str()) + ( atoi(seed.substr(1, seed.size() - 1).c_str()) % ( atoi(n2.c_str())
                                                                                              - atoi(n1.c_str()) + 1 ) );
        //channel.getMutex().unlock();
//        std::cout << defred<<"[t pull]  " <<interval<<endl<<def1;
//        std::chrono::time_point wakeupAt(std::chrono::system_clock::now() + std::chrono::milliseconds (interval));

        std::chrono::duration<double> wakeupAt = std::chrono::milliseconds (interval);
        while (true) {
            if (getChannel().shouldTerminate()) break;
            std::unique_lock<std::mutex> lk(getChannel().getMutex());

            if (not getChannel().waitFor_pull(lk, wakeupAt))
            {
//                std::cout << def<<"[t pull] after waitFor_pull " <<endl<<def1;
                lk.unlock();
                counter ++;
                if (getChannel().shouldTerminate()) break;
                else if (this->lemma_pull() and counter % 2 == 0)
                {
                    counter = 0;
                    this->interrupt(PartitionChannel::Command.ClauseInjection);
                }

//                    std::cout << "[t comunication] Wakeup time not reached! " << "\tLemmas:" << pulled_lemmas.size()
//                              << std::endl;
//                wakeupAt = std::chrono::milliseconds ( lemmaIsPulled ? interval: interval / 2);
            }
            else
            {
//                std::cerr << "Break pull : " << std::endl;
                break;
            }
        }
//        std::unique_lock<std::mutex> lk(getChannel().getStopMutex());
//        getChannel().setPullTerminate();
//        if (getChannel().shouldPushTerminate())
//        {
//            lk.unlock();
//            getChannel().notify_one_t();
//        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception pull : " << e.what() << std::endl;
    }
}

void SolverProcess::clausePush(const string & seed, const string & n1, const string & n2)
{
    try {
        timer.start();
        int interval = atoi(n1.c_str()) + ( atoi(seed.substr(1, seed.size() - 1).c_str()) % ( atoi(n2.c_str())
                                                                                              - atoi(n1.c_str()) + 1 ) );

//        std::chrono::time_point wakeupAt(std::chrono::system_clock::now() + std::chrono::milliseconds (interval));
        std::chrono::duration<double> wakeupAt = std::chrono::milliseconds (interval);
        std::map<std::string, std::vector<net::Lemma>> toPush_lemmas;
        std::map<std::string, std::vector<std::pair<string ,int>>> clauses;
        while (true) {
            opensmt::PrintStopWatch timer1("[t push wait for push red]",synced_stream,
                                           true ? Color::Code::FG_RED : Color::Code::FG_DEFAULT );
//            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            if (getChannel().shouldTerminate()) break;
            std::unique_lock<std::mutex> lk(getChannel().getMutex());

            if (not getChannel().waitFor_push(lk, wakeupAt))
            {


#ifdef ENABLE_DEBUGING
                synced_stream.println(true ? Color::FG_BLUE : Color::FG_DEFAULT, "time of sync: ",timer.elapsed_time_milliseconds());
#endif

                synced_stream.println(true ? Color::Code::FG_BLUE : Color::Code::FG_DEFAULT, "time: ",
                                      timer.elapsed_time_milliseconds());
                if (getChannel().shouldTerminate()) break;
                else if (not getChannel().empty())
                {
                    clauses = getChannel().getClauseMap();
                    getChannel().clear();
                    lk.unlock();
                    //Read buffer
                    for ( const auto &toPushClause : clauses )
                    {
                        for (auto clause = toPushClause.second.cbegin(); clause != toPushClause.second.cend(); ++clause) {
                            toPush_lemmas[toPushClause.first].push_back(net::Lemma(clause->first, clause->second));
//                            mtx_listener_solve.lock();
//                            if (clause->first.find("frame0") != string::npos) std::cout<<defred<<clause->first<<def1<<endl<<endl;
//                            else if (clause->first.find("frame") != string::npos) std::cout<<defred<<clause->first<<def1<<endl<<endl;
//                            mtx_listener_solve.unlock();
                        }
                    }

//Synchronize with CC

                    lemma_push(toPush_lemmas);
                    toPush_lemmas.clear();
//                } else std::cout << "\033[1;51m [t comunication] No shared clauses to copy -> size::\033[0m" << endl;
//                    wakeupAt = std::chrono::system_clock::now() + std::chrono::milliseconds (interval);
                }
                else std::cout << "[t push] Channel empty! " <<endl;

            }
            else { break; }
//        wakeupAt = std::chrono::system_clock::now() + minSleep;
        }
//        std::unique_lock<std::mutex> lk(getChannel().getStopMutex());
//        getChannel().shouldPushTerminate();
//
//        if (getChannel().shouldPullTerminate()) {
//            lk.unlock();
//            getChannel().notify_one_t();
//        }
//        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception push : " << e.what() << std::endl;
    }
}
void SolverProcess::checkForlearned_pushBeforIncrementality() {
//    std::scoped_lock<std::mutex> _l(this->lemma.mtx_li);
    if (openSMTSolver->learned_push) {
        openSMTSolver->learned_push = false;
        openSMTSolver->getMainSplitter().pop();
    }
}

void SolverProcess::search()
{
    std::string smtlib;

//    try {
        while (true) {


//            if (getChannel().shouldTerminate()) {
//                break;
//            }

#ifdef ENABLE_DEBUGING
            std::cout << "[t comunication -> PID= "+to_string(getpid())+" ] befor interpFile: "<< std::endl
                <<(char *) (smtlib+ this->header["query"]).c_str()<<endl;
#endif
            getChannel().setWorkingNode(this->header["node"]);
            getChannel().getMutex().unlock();
            openSMTSolver->preInterpret->interpFile((char *) (smtlib + this->header["query"]).c_str());
            if (getChannel().shouldTerminate()) {
                break;
            }
            smtlib.clear();
            getChannel().clearShouldStop();
            std::unique_lock<std::mutex> lock_channel(getChannel().getMutex());
            openSMTSolver->setResult(openSMTSolver->getMainSplitter().getStatus());
            if (openSMTSolver->getResult() == s_True) {
                this->report(PartitionChannel::Status::sat);
            } else if (openSMTSolver->getResult() == s_False) {
                this->report(PartitionChannel::Status::unsat);

            }

            if (openSMTSolver->getResult() != s_Undef) {
                openSMTSolver->setResult (s_Undef);

#ifdef ENABLE_DEBUGING
                //                std::cout <<defred<< "[t comunication -> PID= "+to_string(getpid())+" ]  is waiting to receive command ... ]"<<def1<<endl ;
#endif
                getChannel().waitForQueryOrTemination(lock_channel);
//                std::cout <<defred<< "[t comunication -> PID= "+to_string(getpid())+" ]  after waiting to receive command ... ]"<<def1<<endl ;
//                cv.wait(lock, [&] { return (getChannel().shouldTerminate() or not header_Temp.empty());});
                if (getChannel().shouldTerminate())
                {
                    break;
                }


                std::unique_lock<std::mutex> lock(mtx_listener_solve);
                PartitionChannel::Task task = this->wait(0);
//                std::cout <<defred<< "[t comunication -> PID= "+to_string(getpid())+" ]  after reading incremental params ... ]"<<def1<<endl ;
//                    getChannel().getMutex().lock();
                smtlib = task.smtlib;
                header_Temp.clear();
                instance_Temp.clear();
                getChannel().clear_query();
                getChannel().clearShouldStop();
//                checkForlearned_pushBeforIncrementality();
//                std::cout << "\033[1;51m [t solver is incremental mode after checkForlearned_pushBeforIncrementality]: \033[0m\t" ;

            }
            else //(not getChannel().isEmpty_query())
            {
                std::unique_lock<std::mutex> lock(mtx_listener_solve);
                if (getChannel().isInjection())
                {
                    std::cout <<"Current Node -> "<< this->header["node"] << std::endl;
                    for ( const auto &lemmaPulled : node_PulledLemmas )
                    {
                        std::cout <<"Node -> "<< lemmaPulled.first << std::endl;
                        if (not node_PulledLemmas[lemmaPulled.first].empty())
                        {
                            if (isPrefix(lemmaPulled.first.substr(1, lemmaPulled.first.size() - 2),
                                         this->header["node"].substr(1, this->header["node"].size() - 2)))
                                injectPulledClauses(lemmaPulled.first);
                        }
                    }

                    node_PulledLemmas.clear();
                    getChannel().clearClauseInjection();
                }

                if (not getChannel().isEmpty_query())
//                for (int index = 0; index < getChannel().size_query(); index++)
                {
                    if (getChannel().shouldTerminate()) {
                        break;
                    }
                    PartitionChannel::Task task = this->wait(0 );
//                    std::cout<<index<<endl;
//                    if(task.command==Task::stop) break;
                    switch (task.command) {
                        case PartitionChannel::Task::incremental:
                            smtlib = task.smtlib;
//                                checkForlearned_pushBeforIncrementality();
//                                getChannel().setShouldStop();
////
//                                openSMTSolver->preInterpret->interpFile((char *) (smtlib + this->header["query"]).c_str());
//                                smtlib.clear();
////
//                                getChannel().clearShouldStop();
                            break;
                        case PartitionChannel::Task::resume:
                            break;
                    }
//                        std::unique_lock<std::mutex> lk(getChannel().getStopMutex());
//                        getChannel().clearShouldStop();
                    //getChannel().notify_one();
                    getChannel().pop_front_query();
//                    std::scoped_lock<std::mutex> _l(this->mtx_listener_solve);
                    header_Temp.pop_front();
                    instance_Temp.pop_front();
                }

            }
            if (getChannel().size_query() > 0)
                getChannel().setShouldStop();
            else getChannel().clearShouldStop();



#ifdef ENABLE_DEBUGING

#endif
        }
//    }
//    catch (std::exception& e)
//    {
//        std::cerr << "Exception search : " << e.what() << std::endl;
//    }
}

void SolverProcess::interrupt(const std::string& command) {
#ifdef ENABLE_DEBUGING
    std::cout <<"[t Listener , Pull -> PID= "+to_string(getpid())+" ] OpenSMT2 Should  -> "+command<<endl;
#endif

    if (command == PartitionChannel::Command.Stop) {
        getChannel().setTerminate();
        getChannel().setShouldStop();
        return;
    }
    std::scoped_lock<std::mutex> lk(getChannel().getMutex());
    if (command == PartitionChannel::Command.ClauseInjection) {
        getChannel().setClauseInjection();
    }
    else {
        getChannel().push_back_query(command);
    }
    getChannel().setShouldStop();
}

void SolverProcess::partition(uint8_t n) {
    pid_t pid = getpid();
    //fork() returns -1 if it fails, and if it succeeds, it returns the forked child's pid in the parent, and 0 in the child.
    // So if (fork() != 0) tests whether it's the parent process.
    if (fork() != 0) {
        return;
    }
    std::thread _t([&] {
        while (getppid() == pid)
            sleep(1);
        exit(0);
    });

//    FILE *file = fopen("/dev/null", "w");
//    dup2(fileno(file), fileno(stdout));
//    dup2(fileno(file), fileno(stderr));
//    fclose(file);
//    std::cout << "\033[1;51m [t partition] Inside partition after fork -> size::\033[0m" << endl;
    getChannel().clearClauseShareMode();
    std::vector<std::string> partitions;
    const char *msg;
    if ( not(
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_num,
                                                                   SMTOption(int(n)),
                                                                   msg) and
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_type,
                                                                   SMTOption(this->header.get(net::Header::parameter, "split")
                                                                                     .c_str()),
                                                                   msg) and
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_smt_split_format_length,
                                                                   SMTOption("brief"),
                                                                   msg) and
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_units, SMTOption(spts_time), msg) and
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_inittune, SMTOption(double(2)), msg) and
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_midtune, SMTOption(double(2)), msg) and
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_asap, SMTOption(1), msg)))
    {
        this->report(partitions, msg);
    }
    else {
        label:
        sstat status = openSMTSolver->getMainSplitter().solve();
        if (status == s_Undef) {
            partitions = openSMTSolver->getMainSplitter().getSolverPartitions();
#ifdef ENABLE_DEBUGING
            std::cout <<"Recieved PN from SMTS Server-> "<<int(n)<<"\tSplits constructed by solver: "<<partitions.size()<<endl;
#endif

//            if (partitions.size() <= 1) {
//                partitions.clear();
//                goto label;
//            }
//            else
            this->report(partitions);
        } else if (status == s_True) {
#ifdef ENABLE_DEBUGING
            std::cout<<"PartitionProcess - Result is SAT"<<endl;
#endif
            this->report(PartitionChannel::Status::sat);
        }
        else if (status == s_False) {
#ifdef ENABLE_DEBUGING
            std::cout<<"PartitionProcess - Result is UNSAT"<<endl;
#endif
            Logger::writeIntoFile(false, this->header["node"] ,"Result is UNSAT ",getpid());
            this->report(PartitionChannel::Status::unsat);
        }
        else {
            this->report(partitions, "error during partitioning");
        }
    }
//    std::cout <<"PartitionProcess - Main Thread: End to partition process"<<endl;
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
        openSMTSolver.reset( new OpenSMTSolver(config, instance, getChannel()));
        openSMTSolver->preInterpret->interpFile((char *) (payload + header["query"]).c_str());

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

void SolverProcess::injectPulledClauses(const std::string& nodePath) {
//    if (pulled_lemmas[nodePath].empty()) return;
//    if (openSMTSolver->learned_push)
//        openSMTSolver->getMainSplitter().pop();

#ifdef ENABLE_DEBUGING
    cout << "\033[1;32m [t comunication -> PID= "+to_string(getpid())+" ] inject accumulated clauses -> Size::\033[0m" << node_PulledLemmas[nodePath].size() << std::endl;
#endif
//    openSMTSolver->getMainSplitter().push();
//    openSMTSolver->learned_push = true;
//    try
//    {
        for (auto &lemma:node_PulledLemmas[nodePath])
        {
            if (lemma.smtlib.size() > 0)
            {

#ifdef ENABLE_DEBUGING
                //                if  (lemma.smtlib.size() % 100 == 0)
//                    cout << (char *) ("(assert " + lemma.smtlib + ")").c_str()<<std::endl;
#endif
                openSMTSolver->preInterpret->interpFile((char *) ("(assert " + lemma.smtlib + ")").c_str());

            }
        }
//    }
//    catch (std::exception& e)
//    {
//        std::cerr << "Exception injectPulledClauses : " << e.what() << std::endl;
//    }
}
