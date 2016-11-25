//
// Created by Matteo on 22/07/16.
//

#ifndef CLAUSE_SHARING_SOLVERSERVER_H
#define CLAUSE_SHARING_SOLVERSERVER_H

//#include <ctime>
#include "lib/net.h"
#include "SolverProcess.h"


class SolverServer : public net::Server {
private:
    void stop_solver();

    void update_lemmas();

    void log(uint8_t, std::string, std::map<std::string, std::string> *_ = nullptr);

    bool check_header(std::map<std::string, std::string> &);

    net::Socket server;
    std::string lemmas_address;
    SolverProcess *solver;
protected:
    void handle_close(net::Socket &);

    void handle_exception(net::Socket &, net::SocketException &);

    void handle_message(net::Socket &, std::map<std::string, std::string> &, std::string &);

public:
    SolverServer(const net::Address &);

    ~SolverServer();
};


#endif //CLAUSE_SHARING_SOLVERSERVER_H
