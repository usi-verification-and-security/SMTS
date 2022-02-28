
#ifndef SMTS_LIB_SMTSTHREAD_H
#define SMTS_LIB_SMTSTHREAD_H

#include <thread>
#include <unordered_map>
#include "lib/lib.h"
#include "Stoppable.hpp"
//#include "lib/HackingSTL.h"

class SMTSThread {
private:
//    typedef std::unordered_map<std::string, pthread_t> SMTSThreadMap;
//    SMTSThreadMap tm_;
    // Create a vector of threads
    std::vector<std::thread> vecOfThreads;

protected:
    net::Socket* SMTSServer;

    virtual void main() = 0;
    virtual void clausePush(const string & seed, const string & n1, const string & n2) = 0;
    virtual void clausePull(const string & seed, const string & n1, const string & n2) = 0;
    virtual void memoryCheck() = 0;
public:

    void wait_ForThreads()
    {
        for (std::thread& th : vecOfThreads) {
            if (th.joinable())
                th.join();
        }
    }
    void worker(int tname, const string& seed, const string& td_min, const string& td_max) {

        switch (tname) {
            case PartitionChannel::ThreadName::Comunication:
#ifdef ENABLE_DEBUGING
                std::cout << "[t Listener] -> PID= "+std::to_string(getpid())+" ] Comunication Thread Started" << std::endl;
#endif
                main();
#ifdef ENABLE_DEBUGING
                std::cout << "[t Listener] -> PID= "+std::to_string(getpid())+" ] Comunication Thread Terminated" << std::endl;
#endif
                break;

            case PartitionChannel::ThreadName::ClausePush:
#ifdef ENABLE_DEBUGING
                std::cout << "[t Listener] -> PID= "+std::to_string(getpid())+" ] Push Thread Started" << std::endl;
#endif
                clausePush(seed, td_min, td_max);
#ifdef ENABLE_DEBUGING
                std::cout << "[t Listener] -> PID= "+std::to_string(getpid())+" ] Push Thread Terminated" << std::endl;
#endif
                break;

            case PartitionChannel::ThreadName::ClausePull:
#ifdef ENABLE_DEBUGING
                std::cout << "[t Listener] -> PID= "+std::to_string(getpid())+" ] Pull Thread Started" << std::endl;
#endif
                clausePull(seed, td_min, td_max);
#ifdef ENABLE_DEBUGING
                std::cout << "[t Listener] -> PID= "+std::to_string(getpid())+" ] Pull thread Terminated" << std::endl;
#endif
                break;
            case PartitionChannel::ThreadName::MemCheck:
#ifdef ENABLE_DEBUGING
                std::cout << "[t Listener] -> PID= "+std::to_string(getpid())+" ] MemCheck Thread Started" << std::endl;
#endif
                memoryCheck();
#ifdef ENABLE_DEBUGING
                std::cout << "[t Listener] -> PID= "+std::to_string(getpid())+" ] MemCheck thread Terminated" << std::endl;
#endif
                break;
        }

    }
    void start_Thread(PartitionChannel::ThreadName tname, const string& seed = std::string(), const string& td_min = std::string(),
                      const string& td_max = std::string())
    {
//        std::thread th = std::stacking_thread(8*1024*1024, &SMTSThread::worker, this, tname, seed, td_min, td_max);
        std::thread th( &SMTSThread::worker, this, tname, seed, td_min, td_max);
        vecOfThreads.push_back( std::move(th) );
    }
//    void forceStop_Thread(const std::string &tname)
//    {
//        SMTSThreadMap::const_iterator it = tm_.find(tname);
//        if (it != tm_.end()) {
//            pthread_cancel(it->second);
//            tm_.erase(tname);
//            std::cout << "SMTSThread " << tname << " killed:" << std::endl;
//        }
//    }

    virtual ~SMTSThread() = default;

    net::Socket *reader() const { return  this->SMTSServer; };

    net::Socket *writer() const { return this->SMTSServer; };

};


#endif