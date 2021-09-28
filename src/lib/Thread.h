
#ifndef SMTS_LIB_THREAD_H
#define SMTS_LIB_THREAD_H

#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include "Exception.h"
#include "net.h"
#include <unordered_map>
//#include "lib/Thread_pool.hpp"
#include "Stoppable.hpp"
#include "unistd.h"
#include "string"


class Thread : public Stoppable {
private:

    //  function which points back to the instance
     void worker(const std::string tname) {
//try {
    if (tname == "solver") {
//        std::cout << "Task Start" << std::endl;
        // Check if thread is requested to stop ?
//             while (stopRequested() == false)
        {
//            std::cout << "Doing solve Work" << std::endl;
            main();
            std::cout << "t Listener -> PID= "+std::to_string(getpid())+" ] Comunication thread Terminated" << std::endl;
//                 std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
//        std::cout << "Task End" << std::endl;

    } else if (tname == "clausePush") {
//        std::cout << "Task Start" << std::endl;
        // Check if thread is requested to stop ?
//             while (stopRequested() == false)
        {
//            std::cout << "Doing clausePush Work" << std::endl;
            clausePush();
            std::cout << "t Listener -> PID= "+std::to_string(getpid())+" ] Push thread Terminated" << std::endl;
//                 std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
//        std::cout << "Task End" << std::endl;

    } else if (tname == "clausePull") {
//        std::cout << "Task Start" << std::endl;
        // Check if thread is requested to stop ?
//             while (stopRequested() == false)
        {
//            std::cout << "Doing clausePull Work" << std::endl;
            clausePull();
            std::cout << "t Listener -> PID= "+std::to_string(getpid())+" ] Pull thread Terminated" << std::endl;
//                 std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
//        std::cout << "Task End" << std::endl;

    }
//}
//catch (...) {
//    std::cout << "exception exec " << std::endl;
//}
    }

    //typedef std::unordered_map<std::string, std::thread*> ThreadMap;
    typedef std::unordered_map<std::string, pthread_t> ThreadMap;
    ThreadMap tm_;
    // Create a vector of threads
    std::vector<std::thread> vecOfThreads;

protected:
    virtual void main() { };
    virtual void clausePush() { } ;
    virtual void clausePull() { } ;

public:
    net::Socket* smtsServer;
    /**
     * Waits for the thread to terminate.
     **/
//    inline void join() {
//        if (nullptr != uthread) {
//            uthread->join();
//            uthread = nullptr;
//        }
//    }

    void start_thread(const std::string &tname)
    {
//        try {
//            if (tname != "main") {
//                std::thread *thrd = new std::thread(&Thread::exec, this, tname);
//                tm_[tname] = thrd->native_handle();
//                thrd->detach();
//            }
//            else{
                std::thread th(&Thread::worker, this, tname);
                vecOfThreads.push_back(std::move(th));
//                reverse(vecOfThreads.begin(), vecOfThreads.end());
//                }
//        thrd->join();

            //tm_[tname] = thrd;
        std::cout << "t Listener -> PID= "+std::to_string(getpid())+" ] Thread " << tname << " Started" << std::endl;
//        }
//        catch (...) {
//            std::cout << "exception start_thread threads start" << std::endl;
//        }
    }
//    pthread_t findThread(const std::string &tname) const
//    {
//       return tm_.find(tname)->second;
//    }
    void stop_thread(const std::string &tname)
    {
        ThreadMap::const_iterator it = tm_.find(tname);
        if (it != tm_.end()) {
//            delete it->second; // thread not killed
            //it->second->std::thread::~thread(); // thread not killed
            pthread_cancel(it->second);
            tm_.erase(tname);
            std::cout << "Thread " << tname << " killed:" << std::endl;
        }
    }

    Thread();

    virtual ~Thread();

    net::Socket *reader() const;

    net::Socket *writer() const;

};


#endif
