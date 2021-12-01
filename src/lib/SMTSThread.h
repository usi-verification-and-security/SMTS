
#ifndef SMTS_LIB_SMTSTHREAD_H
#define SMTS_LIB_SMTSTHREAD_H

#include <thread>
#include <unordered_map>
#include "lib/lib.h"
#include "Stoppable.hpp"
//#include "lib/Thread_pool.hpp"

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

public:

    void wait_ForThreads()
    {
        for (int i = vecOfThreads.size() - 1; i >= 0; --i) {
            if (vecOfThreads[i].joinable())
                vecOfThreads[i].join();
        }
    }
    void worker(int tname, const string& seed, const string& td_min, const string& td_max) {

        switch (tname) {
            case PartitionChannel::ThreadName::Comunication:
                std::cout << "[t Listener] -> PID= "+std::to_string(getpid())+" ] Comunication Thread Started" << std::endl;
                main();
                std::cout << "[t Listener] -> PID= "+std::to_string(getpid())+" ] Comunication Thread Terminated" << std::endl;
                break;

            case PartitionChannel::ThreadName::ClausePush:
                std::cout << "[t Listener] -> PID= "+std::to_string(getpid())+" ] Push Thread Started" << std::endl;
                clausePush(seed, td_min, td_max);
                std::cout << "[t Listener] -> PID= "+std::to_string(getpid())+" ] Push Thread Terminated" << std::endl;
                break;

            case PartitionChannel::ThreadName::ClausePull:
                std::cout << "[t Listener] -> PID= "+std::to_string(getpid())+" ] Pull Thread Started" << std::endl;
                clausePull(seed, td_min, td_max);
                std::cout << "[t Listener] -> PID= "+std::to_string(getpid())+" ] Pull thread Terminated" << std::endl;
                break;
        }

    }
    void start_Thread(PartitionChannel::ThreadName tname, const string& seed = std::string(), const string& td_min = std::string(),
                      const string& td_max = std::string())
    {
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