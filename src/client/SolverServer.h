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

    net::Socket server;
    std::string lemmas_address;
    SolverProcess *solver;
protected:
    void handle_close(net::Socket &);

    void handle_exception(net::Socket &, const net::SocketException &);

    void handle_message(net::Socket &, net::Header &, std::string &);

public:
    SolverServer(const net::Address &);

    ~SolverServer();
};


#endif
