
#include "Thread.h"


Thread::Thread(){}

Thread::~Thread() {
//    stop();
    for (int i = vecOfThreads.size() - 1; i >= 0; --i) {
        if (vecOfThreads[i].joinable())
            vecOfThreads[i].join();
    }
//    for (std::thread & th : vecOfThreads)
//    {
//        std::cout << "thread solver is joining" << std::endl;
//        // If thread Object is Joinable then Join that thread.
//        if (th.joinable())
//            th.join();
//    }
//    stop_thread("solver");
//    stop_thread("clauseShare");
////    stop_thread("solver");
//    stop_thread("clausePull");
    std::cout << "All threads are jointed!!" << std::endl;
//    stop();
//    std::cout << "after thread stop" << std::endl;
//    for (auto& th: tm_)
//    {
//        std::cout << "join thread stop" << std::endl;
//        th.second->join();
//        free(th.second);
//    }

//    ThreadMap::const_iterator it = tm_.find("solver");
//    if (it != tm_.end()) {
//
//    }
//    stop_thread("clauseShare");
//    stop_thread("solve");
//    stop_thread("solver");
//    this->stop();
//    this->join();
//    delete this->thread;
//    this->thread = nullptr;
}


net::Socket* Thread::reader() const {
    return  this->smtsServer ;
//    return (pthread_self() == findThread("solver")) ? this->piper.reader() : this->pipew.reader();
}

net::Socket* Thread::writer() const {
    return this->smtsServer;
//    return (pthread_self() != findThread("solver")) ? this->pipew.writer() : this->piper.writer();
}
