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
#include "OsmtInternalException.h"

using namespace opensmt;

std::unique_ptr<OpenSMTSolver> openSMTSolver;
const char *SolverProcess::solver = "OpenSMT2";
//Color::Modifier def(Color::BG_GREEN);
//std::mutex mtx_solve;

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
    const char *msg;
    SMTConfig config;

    config.setRandomSeed(atoi(this->header.get(net::Header::parameter, "seed").c_str()));

//    Logger::writeIntoFile(false,
//                          "Solver -> instance: "
//            ,(char *) (smtlib + this->header["query"]).c_str()
//            ,getpid());
    openSMTSolver.reset( new OpenSMTSolver(this->header, config,  this->instance, getChannel()));

    if (lemma_server) {
        getChannel().setClauseShareMode();
        start_thread("clausePush");
        start_thread("clausePull");
    }
    search();
}

//void wait_pause(std::unique_lock<std::mutex> & lock)
//{
//    return cv.wait(lock, [&] { return not requestStop; });
//}
//void SolverProcess::stop()
//{

//Lemma Server Thread Pool Architecture:
    //Dynamically construct a thread pool which employed as much threads as a machine can handle in parallel.
    //A queue of tasks , technically function pointer should be implemmented, comming task shall be pushed into the queue, if queue's waiting tasks are
    //bigger than pools, task should wait till get notified.
    //Each idle thread shall wait for 1000 MicroSecond before procceding to the next task and meanwhile check for termination signal:
    //If set, yield and join.
    //N.B** All threads of execution are running the exact same code with different socket input and payload
    // Read and write sockets can be: Shared or Seperate. Seperate would meet Acr draft.
    //System-Wide locking mechanism on queue could backfire, Shall be avoided... Parallel behaviour hare expected!
    //Take lock only on shared resources, class-wide level variables, map of solvers, for instance
    // Variales shall be local to avoid lock and overhead.
    //scoped_lock on the queue can be considered providing there is quite robust synchronizing mechanism so no data would be overrided.

//Thread Architecure, SMTS CLient
    //Include Five Thread sharing memmory
    //SMTS Thread:
    //Always listening to SMTS Server , Blocking
    // Before registering solver existence to the server, Channel should be created.
    //A reference of the Channel shall be passed to the Comunication Thread through the constructor of the comunication once
    //Once solve command is pushed from python server, then SMTS Thread will fire another thread as Comunication Thread.
    //To the Comunication, T Channel should be passed as well as Header and Base Instance.
    // Comunication::Comunication( Channel & channel )
    //Comunication Thread
    //Comunication T is responsible for all operation inside SMTS clients, solvers.
    //**N.B
    //SMTS and Comunication T shall not comunicate directly but rather through the channel.
    //That is, firing another purely solving T so OpenSMT2 can be run upon.
    //Comunication T is the Only T which is able to command Solve Thread To exit.
    //Comunication T shall wait on several condition through channel, based on pushed query, pulled clauses and timeout ponit
    //After Comunication T would command openSMT2 to exit then Comunication T would follow 4 possible scenario:
    //1. to execute pushed query from SMTS T which can be:
    //1.1 Partition: while solver is proceding can fire a parallel process to handle partition.
    //1.2 Incrementality: Split partitions which are reported to the SMTS Server, are pushed to solvers to work on.
    //1.3 Stop: thread Arc shall be canceled immediately upon stop command, exept SMTS T.
    //2. to push pulled clauses into solver by Pull Thread which would be running once a min, configurable
    //Accumulation shall be implemented, to make sure all clauses are pushed to LS.
    //3. to push learnt clauses to the Lemma Server.
    //Accumulation shall be implemented, to make sure all clauses correspondent to solvers are pulled from LS.
    //Thread of Lemma Push
    // It'll check the channel for existing learnt clauses.
    // Connect to the lema server to write its state
    //Thread of Lemma Pull
    //Ping every now and then Lemmas server, read some lemmas to be pushed to the Solver.

    //Bugs *******
    //Assertion error: on some clause which contains .frameN
    // Solution: Clauses with frame sshould not be pushed at the first place. They should be filtered through old and new clasuses.
    // Partition could be done through forked process or the same solver thread
    //pros and cons: Forked process would inherit only one thread upon which is created so managing which thread owning the solver state
    //at the time of fork is quite complex! on the other hand using the same thread would make solver state inconsistent
    // Backtracking or Antti suggestion to push and pop could be helpful.
    //Learnt data base clause could be immplemented to avoid overhead.
    //Channel should be independent, and passed to the OpenSMT through constructors.
    // What is filter heuristic in SMTS, server Arc?
    //https://www.inf.usi.ch/postdoc/hyvarinen/publications/HyvarinenMAS_SAT2016.pdf
//}

void SolverProcess::clausePull()
{
    try {
//        time_t start;
//        time_t finish;
        int counter = 0;
//        srand(getpid() + time(NULL));

        int interval =lemmaPull_timeout_min +
                ( atoi(this->header.get(net::Header::parameter, "seed").c_str()) % ( lemmaPull_timeout_max - lemmaPull_timeout_min + 1 ) );
//        std::cout << interval << endl;
//        std::chrono::time_point wakeupAt(std::chrono::system_clock::now() + std::chrono::milliseconds (interval));
        std::chrono::duration<double> wakeupAt = std::chrono::milliseconds (interval);
        bool lemmaIsPulled = false;
        while (true) {
            std::unique_lock<std::mutex> lk(getChannel().getMutex());
//            std::cout << "[t comunication] before waitFor_pull " <<endl;
            if (not getChannel().waitFor_pull(lk, wakeupAt))
            {
//                std::cout << "[t comunication] after waitFor_pull " <<endl;
                lk.unlock();
                counter ++;
                if (this->lemma_pull())
                {
                    lemmaIsPulled= true;
                }
                if (counter % 2 == 0 and lemmaIsPulled) {
                     {
                         counter=0;
                        lemmaIsPulled= false;
                        this->interrupt("inject");
                        std::cout << "[t pull] Signal to OpenSMT to stop " << "\tLemmas:" << pulled_lemmas.size()
                                  << std::endl;

                    }
                }
//                    std::cout << "[t comunication] Wakeup time not reached! " << "\tLemmas:" << pulled_lemmas.size()
//                              << std::endl;
//                wakeupAt = std::chrono::milliseconds ( lemmaIsPulled ? interval: interval / 2);
            }
            else
            {
//                std::cout << "[t comunication] after waitFor_pull " <<endl;
//                std::cerr << "Break pull : " << std::endl;
                break;
            }
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception pull : " << e.what() << std::endl;
    }
}
void SolverProcess::clausePush()
{
    try {
        int interval = lemmaPush_timeout_min + ( atoi(this->header.get(net::Header::parameter, "seed")
                                             .substr( this->header.get(net::Header::parameter, "seed").length() - 4 ).c_str()) %
                                                     ( lemmaPush_timeout_max - lemmaPush_timeout_min + 1 ) );
//        std::chrono::time_point wakeupAt(std::chrono::system_clock::now() + std::chrono::milliseconds (interval));
        std::chrono::duration<double> wakeupAt = std::chrono::milliseconds (interval);
//        std::cout<<interval<<endl;
        std::vector<net::Lemma> toPush_lemmas;
        while (true) {
//            std::this_thread::sleep_for(std::chrono::milliseconds(interval));

            std::unique_lock<std::mutex> lk(getChannel().getMutex());
//            std::cout << "[t comunication] before waitFor_push " <<endl;
            if (getChannel().waitFor_push(lk, wakeupAt))
            {
//                std::cout << "[t comunication] after waitFor_push " <<endl;
                if (getChannel().shouldTerminate())
                {
//                    std::cerr << "Break push : " << std::endl;
                    break;
                }
                else if (not getChannel().empty())
                {
                    //Read buffer

                    for (auto clause = getChannel().cbegin(); clause != getChannel().cend(); ++clause) {
                        toPush_lemmas.push_back(net::Lemma(clause->first, clause->second));
                    }
                    getChannel().clear();
//            getChannel().clearClauseReady();

//Synchronize with CC
                    lk.unlock();
                    lemma_push(toPush_lemmas);
                    toPush_lemmas.clear();
//                } else std::cout << "\033[1;51m [t comunication] No shared clauses to copy -> size::\033[0m" << endl;
//                    wakeupAt = std::chrono::system_clock::now() + std::chrono::milliseconds (interval);
                }
            }
//            else{ std::cout << "[t comunication] after waitFor_push " <<endl;}
//        wakeupAt = std::chrono::system_clock::now() + minSleep;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception push : " << e.what() << std::endl;
    }
}
void SolverProcess::checkForlearned_pushBeforIncrementality() {
    std::scoped_lock<std::mutex> _l(this->lemma.mtx_li);
    if (openSMTSolver->learned_push) {
        openSMTSolver->learned_push = false;
        openSMTSolver->getMainSplitter().pop();
    }
}

void SolverProcess::search()
{
    std::string smtlib;
    try {
        while (true) {
            if (getChannel().shouldTerminate()) {
                break;
            }
            string query=this->header["query"];
//            std::cout << (char *) (smtlib + this->header["query"]).c_str() << endl;
            getChannel().getMutex().unlock();
            std::cout << "[t comunication -> PID= "+to_string(getpid())+" ] Payload: "<< std::endl<<(char *) (smtlib + query).c_str();
            openSMTSolver->interpret->interpFile((char *) (smtlib + query).c_str());

            if (getChannel().shouldTerminate()) {

                break;
            }
            smtlib.clear();
            getChannel().clearShouldStop();
            std::unique_lock<std::mutex> lock_channel(getChannel().getMutex());

            openSMTSolver->result = openSMTSolver->getMainSplitter().getStatus();
            if (openSMTSolver->result == s_True) {
                this->report(Status::sat);
            } else if (openSMTSolver->result == s_False) {
                this->report(Status::unsat);

            }
            std::unique_lock<std::mutex> lock(mtx_listener_solve);
            if (openSMTSolver->result != s_Undef) {
                openSMTSolver->result = s_Undef;

//                getChannel().waitForQueryOrTemination(lock_channel);

                lock_channel.unlock();

                cv.wait(lock, [&] { return (getChannel().shouldTerminate() or not header_Temp.empty());});
                lock_channel.lock();


                if (getChannel().shouldTerminate())
                {
                    break;
                }
//                lock.unlock();
                std::cout << "\033[1;51m [t solver is incremental mode]: \033[0m\t" ;

                getChannel().clear_query();
                getChannel().clearShouldStop();

                Task task = this->wait(0);

//                    getChannel().getMutex().lock();
                header_Temp.clear();
                smtlib = task.smtlib;
//                checkForlearned_pushBeforIncrementality();
//                std::cout << "\033[1;51m [t solver is incremental mode after checkForlearned_pushBeforIncrementality]: \033[0m\t" ;

            }
            else //(not getChannel().isEmpty_query())
            {

                if (getChannel().isInjection())
                {
//                    std::scoped_lock<std::mutex> _l_lemma(this->lemma.mtx_li);
                    if (isPrefix(currentLemmaPulledNodePath.substr(1,currentLemmaPulledNodePath.size() - 2) ,
                                 this->header["node"].substr(1,this->header["node"].size() - 2)))
                        injectPulledClauses();
                    if(pulled_lemmas.empty()) pulled_lemmas.clear();
                    getChannel().clearClauseInjection();
                }
                if (not getChannel().isEmpty_query())
//                for (int index = 0; index < getChannel().size_query(); index++)
                {
                    if (getChannel().shouldTerminate()) {
                        break;
                    }
                    Task task = this->wait(0 );
//                    std::cout<<index<<endl;
//                    if(task.command==Task::stop) break;
                        switch (task.command) {
                            case Task::incremental:
                                smtlib = task.smtlib;
//                                checkForlearned_pushBeforIncrementality();
//                                getChannel().setShouldStop();
////
//                                openSMTSolver->interpret->interpFile((char *) (smtlib + this->header["query"]).c_str());
//                                smtlib.clear();
////
//                                getChannel().clearShouldStop();
                                break;
                            case Task::resume:
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
            lock.unlock();

            if (getChannel().shouldTerminate()) {
                break;
            }


#ifdef ENABLE_DEBUGING

#endif
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception search : " << e.what() << std::endl;
    }
}

void SolverProcess::interrupt(string command) {
#ifdef ENABLE_DEBUGING
    std::cout <<"interrupt interrupt -> "+command<<endl;
#endif
    if (command == "stop") {
        getChannel().setTerminate();
        getChannel().setShouldStop();
        return;
    }

    std::scoped_lock<std::mutex> lk(getChannel().getMutex());
    if (command == "inject") {
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
#ifdef ENABLE_DEBUGING
    std::cout <<"PartitionProcess - Main Thread: Start to fork partition process"<<endl;
#endif
    if (fork() != 0) {
//        std::this_thread::sleep_for(std::chrono::milliseconds (100));
        return;
    }
    std::thread _t([&] {
//        std::this_thread::sleep_for(std::chrono::seconds (4));
//        exit(0);
        while (getppid() == pid)
            sleep(1);
        exit(0);
    });
//    std::cout << "\033[1;51m [t solve] Inside partition after fork \033[0m"<<header_Temp[0]["command"]<<endl;
//    FILE *file = fopen("/dev/null", "w");
//    dup2(fileno(file), fileno(stdout));
//    dup2(fileno(file), fileno(stderr));
//    fclose(file);
#ifdef ENABLE_DEBUGING
#endif
//    std::cout <<"PartitionProcess - Main Thread: Start to partition process"<<endl;
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
            openSMTSolver->getMainSplitter().getConfig().setOption(SMTConfig::o_sat_split_asap, SMTOption(1), msg)))
    {
        this->report(partitions, msg);
    }
    else {

        sstat status = openSMTSolver->getMainSplitter().solve();
        if (status == s_Undef) {
            partitions = openSMTSolver->getMainSplitter().getSolverPartitions();
#ifdef ENABLE_DEBUGING
            std::cout <<"Recieved PN from SMTS Server-> "<<int(n)<<"\tSplits constructed by solver: "<<partitions.size()<<endl;
#endif

            this->report(partitions);
        } else if (status == s_True) {
#ifdef ENABLE_DEBUGING
            std::cout<<"PartitionProcess - Result is SAT"<<endl;
#endif
            this->report(Status::sat);
        }
        else if (status == s_False) {
#ifdef ENABLE_DEBUGING
            std::cout<<"PartitionProcess - Result is UNSAT"<<endl;
#endif
            this->report(Status::unsat);
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
        openSMTSolver.reset( new OpenSMTSolver(header, config, instance, getChannel()));
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

void SolverProcess::injectPulledClauses() {
//    if (pulled_lemmas.empty()) return;
//    if (openSMTSolver->learned_push)
//        openSMTSolver->getMainSplitter().pop();

    openSMTSolver->getMainSplitter().push();
//    openSMTSolver->learned_push = true;
    cout << "\033[1;32m [t comunication] inject accumulated clauses -> Size::\033[0m"<< pulled_lemmas.size()<<std::endl;
    try
    {
        for (auto &lemma:pulled_lemmas)
        {
            if (lemma.smtlib.size() > 0)
            {
    #ifdef ENABLE_DEBUGING
                //cout << "\033[1;32m [t comunication] clause::\033[0m"<< (char *) ("(assert " + lemma.smtlib + ")").c_str()<<std::endl;
    #endif
                if (getChannel().shouldTerminate()) return;
                openSMTSolver->interpret->interpFile((char *) ("(assert " + lemma.smtlib + ")").c_str());

            }
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception injectPulledClauses : " << e.what() << std::endl;
    }
}