//
// Author: Matteo Marescotti
//

#ifndef SMTS_SERVER_H
#define SMTS_SERVER_H

#include <list>
#include <set>
#include "lib/net/Socket.h"
#include "lib/Thread_pool.hpp"
class Server {
private:
    std::shared_ptr<net::Socket> socket;
    std::set<std::shared_ptr<net::Socket>> sockets;

protected:
    virtual void handle_accept(net::Socket &) {}

    virtual void handle_close(net::Socket &) {}

    virtual void handle_message(net::Socket &, net::Header &, std::string &) {}

    virtual void handle_exception(net::Socket &, const std::exception &) {}
    void task(net::Socket& socket, Server& s);

public:
    Server();

    Server(std::shared_ptr<net::Socket>);

    Server(uint16_t);

    virtual ~Server() {}

    void run_forever();

    void stop();

    void add_socket(std::shared_ptr<net::Socket>);

    void del_socket(std::shared_ptr<net::Socket>);

    void add_socket(net::Socket *);

    void del_socket(net::Socket *);

};

#endif
