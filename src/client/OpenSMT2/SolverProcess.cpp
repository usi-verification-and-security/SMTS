//
// Author: Matteo Marescotti
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
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

    static const char *default_split = spts_none;
    static const char *default_seed = "0";

    if (this->header.get(net::Header::parameter, "seed").size() == 0) {
        this->header.set(net::Header::parameter, "seed", default_seed);
    }

    if (this->header.get(net::Header::parameter, "split-type").size() > 0 &&
        this->header.get(net::Header::parameter, "split-type") != spts_lookahead &&
        this->header.get(net::Header::parameter, "split-type") != spts_scatter) {
        this->warning("bad parameter.split-type: '" + this->header.get(net::Header::parameter, "seed") + "'. using default (" +default_split +")");
        this->header.remove(net::Header::parameter, "split");
    }
    if (this->header.get(net::Header::parameter, "split-type").size() == 0) {
        this->header.set(net::Header::parameter, "split", default_split);
    }
}
void static segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
//    this->error(std::string("Caught segfault at address " + std::to_string(si->si_errno)) +" "+ std::to_string(signal));
    printf("Caught segfault at address %p\n", si->si_errno);
    exit(0);
}
void SolverProcess::solve() {
    const char *msg;
    SMTConfig config;
    config.setRandomSeed(atoi(this->header.get(net::Header::parameter, "seed").c_str()));
    std::cout<<"seed:"<<this->header.get(net::Header::parameter, "seed")<<endl;
    config.setOption(SMTConfig::o_sat_split_type,
                     SMTOption(this->header.get(net::Header::parameter, "split-type").c_str()), msg) ;
#ifdef ENABLE_DEBUGING
    synced_stream.println(true ? Color::FG_BrightCyan : Color::FG_DEFAULT, "split-preference:: "+this->header["split-preference"]);
#endif
    config.setOption(SMTConfig::o_sat_split_preference,SMTOption(this->header["split-preference"].c_str()), msg) ;
    this->header.erase("split-preference");
    openSMTSolver.reset( new OpenSMTSolver(config,  this->instance, getChannel()));

    search();

    try
    {
        if (forked)
            kill_child();
    }
    catch (...) {}
}

void SolverProcess::clausePull(const string & seed, const string & n1, const string & n2)
{
    try {
        timer.start();
//        int counter = 0;
//        srand(getpid() + time(NULL));
        //channel.getMutex().lock();
        int interval = atoi(n1.c_str()) + ( atoi(seed.substr(1, seed.size() - 1).c_str()) % ( atoi(n2.c_str())
                                                                                              - atoi(n1.c_str()) + 1 ) );
        //channel.getMutex().unlock();
//        std::cout << defred<<"[t pull]  " <<interval<<endl<<def1;
//        std::chrono::time_point wakeupAt(std::chrono::system_clock::now() + std::chrono::milliseconds (interval));
        int counter = 0;
        std::chrono::duration<double> wakeupAt = std::chrono::milliseconds (interval);
        while (true) {
            if (getChannel().shouldTerminate()) break;
            std::unique_lock<std::mutex> lk(getChannel().getMutex());
#ifdef ENABLE_DEBUGING
//            opensmt::PrintStopWatch timer1("[t pull wait for pull red]",synced_stream,
//                                           true ? Color::Code::FG_RED : Color::Code::FG_DEFAULT );
#endif
            if (not getChannel().waitFor(lk, wakeupAt))
            {
//                std::cout << def<<"[t pull] after waitFor_pull " <<endl<<def1;
                lk.unlock();
                if (getChannel().shouldTerminate()) break;
                else if (this->lemma_pull() and counter % 1 == 0) {
                    counter = 0;
                    this->interrupt(PartitionChannel::Command.ClauseInjection);
                }
                counter++;

//                    std::cout << "[t comunication] Wakeup time not reached! " << "\tLemmas:" << pulled_lemmas.size()
//                              << std::endl;
//                wakeupAt = std::chrono::milliseconds ( lemmaIsPulled ? interval: interval / 2);
            }
            else
            {
//                std::cerr << "Break pull : " << std::endl;
                break;
            }
#ifdef ENABLE_DEBUGING
//            synced_stream.println(true ? Color::Code::FG_BLUE : Color::Code::FG_DEFAULT, "pull time: ",
//                                  timer.elapsed_time_milliseconds());
#endif
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
#ifdef ENABLE_DEBUGING
//            opensmt::PrintStopWatch timer1("[t push wait for push red]",synced_stream,
//                                           true ? Color::Code::FG_RED : Color::Code::FG_DEFAULT );
#endif
//            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            if (getChannel().shouldTerminate()) break;
            std::unique_lock<std::mutex> lk(getChannel().getMutex());

            if (not getChannel().waitFor(lk, wakeupAt))
            {


#ifdef ENABLE_DEBUGING
//                synced_stream.println(true ? Color::FG_BLUE : Color::FG_DEFAULT, "time of sync: ",timer.elapsed_time_milliseconds());


//                synced_stream.println(true ? Color::Code::FG_BLUE : Color::Code::FG_DEFAULT, "push time: ",
//                                      timer.elapsed_time_milliseconds());
#endif
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
#ifdef ENABLE_DEBUGING
                            if (clause->first.find(".frame") != string::npos) {
                                this->error(std::string("unusual clause found: "));
//                                synced_stream.println(true ? Color::FG_Red : Color::FG_DEFAULT, "[t push frame caught: "+clause->first);
//                                continue;
                            }
#endif
                            toPush_lemmas[toPushClause.first].push_back(net::Lemma(clause->first, clause->second));
                        }
#ifdef ENABLE_DEBUGING
                        synced_stream.println(true ? opensmt::Color::FG_BrightBlue : opensmt::Color::FG_DEFAULT,
                                              "[t push -> PID= "+to_string(getpid())+" ] pushed from Node -> "+ toPushClause.first+" Size:"+ to_string(toPushClause.second.size()));
#endif
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
            std::cout << "[t comunication -> PID= "+to_string(getpid())+" ] Before interpFile: "<< std::endl
                <<(char *) (smtlib+ this->header["query"]).c_str()<<endl;
#endif

            getChannel().setWorkingNode(this->header["node"]);
            getChannel().getMutex().unlock();
//            if (smtlib.find(".ite") != string::npos)
//                this->error(std::string("ite found: ") + smtlib);
//                synced_stream.println(true ? Color::FG_Red : Color::FG_DEFAULT, "[t comunication frame caught in partition: "+ smtlib);
            openSMTSolver->preInterpret->interpFile((char *) (smtlib + this->header["query"]).c_str());

#ifdef ENABLE_DEBUGING
//            std::cout<<"node: "<<this->header["node"]<<endl;
//            for (int i = 0; i <getChannel().size_query(); ++i) {
//                std::cout<<"query "<<i+1<<": "<<getChannel().get_queris()[i]<<endl;
//            }
#endif
            if (getChannel().shouldTerminate()) {
                break;
            }
            smtlib.clear();
            getChannel().clearShouldStop();
            std::unique_lock<std::mutex> lock_channel(getChannel().getMutex());
            openSMTSolver->setResult(openSMTSolver->getMainSplitter().getStatus());
            if (openSMTSolver->getResult() == s_True) {
                std::string statusInfo = openSMTSolver->getMainSplitter().getConfig().getInfo(":status").toString();
                this->report(PartitionChannel::Status::sat, statusInfo);
            } else if (openSMTSolver->getResult() == s_False) {
                std::string statusInfo = openSMTSolver->getMainSplitter().getConfig().getInfo(":status").toString();
                this->report(PartitionChannel::Status::unsat, statusInfo);

            }

            if (openSMTSolver->getResult() != s_Undef) {
                synced_stream.println(true ? Color::FG_Red : Color::FG_DEFAULT, "[t comunication -> result: " + openSMTSolver->getResult().getValue()
                ," from Node " + this->header["node"]);
                openSMTSolver->setResult (s_Undef);

#ifdef ENABLE_DEBUGING
                //                std::cout <<defred<< "[t comunication -> PID= "+to_string(getpid())+" ]  is waiting to receive command ... ]"<<def1<<endl ;
#endif
                getChannel().waitForQueryOrTemination(lock_channel);
//                std::cout <<defred<< "[t comunication -> PID= "+to_string(getpid())+" ]  after waiting to receive command ... ]"<<def1<<endl ;
                if (getChannel().shouldTerminate())
                    break;
                std::unique_lock<std::mutex> lock(mtx_listener_solve);
                if ( getChannel().get_queris()[0] != PartitionChannel::Command.Partition)
                {
                    PartitionChannel::Task task = this->wait(0);
//                std::cout <<defred<< "[t comunication -> PID= "+to_string(getpid())+" ]  after reading incremental params ... ]"<<def1<<endl ;
//                    getChannel().getMutex().lock();
                    smtlib = task.smtlib;
                }
                getChannel().pop_front_query();
                header_Temp.pop_front();
                instance_Temp.pop_front();
                node_PulledLemmas.clear();
                getChannel().clearInjectClause();
//                checkForlearned_pushBeforIncrementality();
//                std::cout << "\033[1;51m [t solver is incremental mode after checkForlearned_pushBeforIncrementality]: \033[0m\t" ;

            }
            else //(not getChannel().isEmpty_query())
            {
                std::unique_lock<std::mutex> lock(mtx_listener_solve);
                if (getChannel().shouldInjectClause())
                {
#ifdef ENABLE_DEBUGING
                    synced_stream.println(true ? opensmt::Color::FG_Cyan : opensmt::Color::FG_DEFAULT,
                                          "[t comunication -> PID= "+to_string(getpid())+" ] Current Node -> "+ this->header["node"] );
#endif
                    for ( const auto &lemmaPulled : node_PulledLemmas )
                    {
#ifdef ENABLE_DEBUGING
                        synced_stream.println(true ? opensmt::Color::FG_Cyan : opensmt::Color::FG_DEFAULT,
                                              "[t comunication -> PID= "+to_string(getpid())+" ] check for pulled Node -> "+ lemmaPulled.first);
#endif
                        if (not node_PulledLemmas[lemmaPulled.first].empty())
                        {
                            if (isPrefix(lemmaPulled.first.substr(1, lemmaPulled.first.size() - 2),
                                         this->header["node"].substr(1, this->header["node"].size() - 2))) {
#ifdef ENABLE_DEBUGING
                                synced_stream.println(true ? opensmt::Color::FG_Cyan : opensmt::Color::FG_DEFAULT,
                                                      "[t comunication -> PID= " + to_string(getpid()) +
                                                      " ] Node is prefix of the current -> " + lemmaPulled.first);
#endif
                                std::string conflict = std::to_string(( (ScatterSplitter &) openSMTSolver->getMainSplitter().getSMTSolver() ).conflicts );
                                std::cout<<";conflict before splitting: "<< conflict<<endl;
                                injectPulledClauses(lemmaPulled.first);
                            }
                        }
                    }

                    node_PulledLemmas.clear();
                    getChannel().clearInjectClause();
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
    synced_stream.println(true ? opensmt::Color::FG_Yellow : opensmt::Color::FG_DEFAULT,
                          "[t Listener , Pull -> PID= "+to_string(getpid())+" ] OpenSMT2 Should  -> "+command);
#endif
    if (getChannel().shouldTerminate()) return;
    if (command == PartitionChannel::Command.Stop) {
        getChannel().setTerminate();
        getChannel().setShouldStop();
        return;
    }
    std::scoped_lock<std::mutex> lk(getChannel().getMutex());
    if (command == PartitionChannel::Command.ClauseInjection) {
        getChannel().setInjectClause();
    }
    else {
        getChannel().push_back_query(command);
    }
    getChannel().setShouldStop();
}
void SolverProcess::kill_child()
{
    int wstatus;
//    printf("printed from parent process - %d\n", getpid());
    std::cout<<"send kill signall to forked process!: "<<child_pid<<endl;
    int ret;
    ret = kill(child_pid, SIGKILL);
    if (ret == -1) {
        std::cout<<"ret == -1: "<<child_pid<<endl;
        perror("kill");
        exit(EXIT_FAILURE);
    }

    if (waitpid(child_pid, &wstatus, WUNTRACED | WCONTINUED) == -1) {
        std::cout<<"waitpid: "<<child_pid<<endl;
        perror("waitpid");
        exit(EXIT_FAILURE);
    }
}
volatile sig_atomic_t shutdown_flag = 1;

void cleanupRoutine(int signal_number)
{
    shutdown_flag = 0;
}
void SolverProcess::partition(uint8_t n) {

    pid_t pid = getpid();
    //fork() returns -1 if it fails, and if it succeeds, it returns the forked child's pid in the parent, and 0 in the child.
    // So if (fork() != 0) tests whether it's the parent process.
    child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (child_pid > 0)
    {
        forked=true;
        return;
    }
    std::thread _t([&] {
        while (getppid() == pid)
            sleep(1);
        exit(0);
    });
    printf("printed from child process - %d\n", getpid());

    struct sigaction sigterm_action;
    memset(&sigterm_action, 0, sizeof(sigterm_action));
    sigterm_action.sa_handler = &cleanupRoutine;
    sigterm_action.sa_flags = 0;

    // Mask other signals from interrupting SIGTERM handler
    if (sigfillset(&sigterm_action.sa_mask) != 0)
    {
        std::cout<<";sigfillset: "<<endl;
        perror("sigfillset");
        exit(EXIT_FAILURE);
    }
    // Register SIGTERM handler
    if (sigaction(SIGTERM, &sigterm_action, NULL) != 0)
    {
        std::cout<<";sigaction SIGTERM: "<<endl;
        perror("sigaction SIGTERM");
        exit(EXIT_FAILURE);
    }

//    while (shutdown_flag1) {
//        count += 1;
//        sleep(1);
//        std::cout<<"from child..."<<"\n";
//    }
//    printf("count = %d\n", count);
//
//    exit(EXIT_SUCCESS);
//    FILE *file = fopen("/dev/null", "w");
//    dup2(fileno(file), fileno(stdout));
//    dup2(fileno(file), fileno(stderr));
//    fclose(file);
//    std::cout << "\033[1;51m [t partition] Inside partition after fork -> size::\033[0m" << endl;
    getChannel().clearClauseShareMode();
    std::vector<std::string> partitions;
    std::string conflict = std::to_string(( (ScatterSplitter &) openSMTSolver->getMainSplitter().getSMTSolver() ).conflicts );
    std::string statusInfo = openSMTSolver->getMainSplitter().getConfig().getInfo(":status").toString();
    std::cout<<";conflict before splitting: "<< conflict<<endl;
    std::cout<<";statusInfo before splitting: "<< statusInfo<<endl;
    const char *msg;
    if ( not(
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_num, SMTOption(int(n)),msg) and
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_solver_limit,SMTOption(INT_MAX),msg) and
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_smt_split_format_length,SMTOption("brief"),msg) and
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_units, SMTOption(spts_time), msg) and
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_inittune, SMTOption(double(2)), msg) and
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_midtune, SMTOption(double(2)), msg) and
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_asap, SMTOption(0), msg)))
    {
        this->report(partitions, conflict, statusInfo, msg);
    }
    else {

        ( (ScatterSplitter &) openSMTSolver->getMainSplitter().getSMTSolver() ).setSplitConfig_split_on();

        sstat status = openSMTSolver->getMainSplitter().solve();
//        if (this->header["node"]== "[0, 0]") {
//            std::cout<<"test"<<endl;
//            this->report(PartitionChannel::Status::unsat);
//            exit(0);
//        }
        if (status == s_Undef) {
            partitions = openSMTSolver->getMainSplitter().getPartitionSplits();
#ifdef ENABLE_DEBUGING
            std::cout <<"Recieved PN from SMTS Server-> "<<int(n)<<"\tSplits constructed by solver: "<<partitions.size()<<this->header["node"]<<endl;
#endif
//            partitions.erase(partitions.begin());
            this->report(partitions, conflict, statusInfo);
        } else if (status == s_True) {
#ifdef ENABLE_DEBUGING
            std::cout<<"PartitionProcess - Result is SAT"<<endl;
#endif
            this->report(PartitionChannel::Status::sat, statusInfo, conflict);
        }
        else if (status == s_False) {
#ifdef ENABLE_DEBUGING
            std::cout<<"PartitionProcess - Result is UNSAT"<<endl;
#endif
//            Logger::writeIntoFile(false, this->header["node"] ,"Result is UNSAT ",getpid());
            this->report(PartitionChannel::Status::unsat, statusInfo, conflict);
        }
        else {
            this->report(partitions, conflict, statusInfo, "error during partitioning");
        }
    }
//    std::cout <<"PartitionProcess - Main Thread: End to partition process"<<endl;
    exit(EXIT_SUCCESS);
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
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_sigaction;
    sa.sa_flags   = SA_SIGINFO;
    sigaction(SIGBUS, &sa, NULL);
#ifdef ENABLE_DEBUGING
    synced_stream.println(true ? opensmt::Color::FG_Green : opensmt::Color::FG_DEFAULT,
                          "[t comunication -> PID= "+to_string(getpid())+" ] inject accumulated clauses -> Size:" + to_string(node_PulledLemmas[nodePath].size()));
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
