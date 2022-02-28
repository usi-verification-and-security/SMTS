//
// Author: Matteo Marescotti
//

#ifndef SMTS_CLIENT_SOLVERSERVER_H
#define SMTS_CLIENT_SOLVERSERVER_H

//#include <ctime>
#include "lib/net.h"
#include "SolverProcess.h"


class SolverServer : public net::Server {
private:
    void stop_solver();

    void update_lemmas();

    void log(uint8_t, std::string);

    net::Socket SMTSServer;
//    net::Socket lemmaServer;
    std::string lemmaServerAddress;
    SolverProcess* solver;
    Channel channel;

protected:
    void handle_close(net::Socket &);

    void handle_exception(net::Socket &, const std::exception &);

    void handle_message(net::Socket &, net::Header &, std::string &);

public:

//    struct {
//        const uint8_t errors_max = 3;
//        std::unique_ptr<net::Socket> server;
//        uint8_t errors = 0;
//        uint8_t interval = 3;
//        std::time_t last_push = 0;
//        std::time_t last_pull = 0;
//        mutable std::mutex lemma_mutex;
//    } lemma;
    SolverServer(const net::Address &);

    ~SolverServer();
};


#endif
