//
// Created by Matteo on 12/08/16.
//

#ifndef CLAUSE_SERVER_SERVER_H
#define CLAUSE_SERVER_SERVER_H

#include <list>
#include <set>
#include "Socket.h"


namespace net {
    class Server {
    private:
        std::shared_ptr<Socket> socket;
        std::set<std::shared_ptr<Socket>> sockets;

    protected:
        virtual void handle_accept(Socket &) {}

        virtual void handle_close(Socket &) {}

        virtual void handle_message(Socket &, net::Header &, std::string &) {}

        virtual void handle_exception(Socket &, SocketException &) {}

    public:
        Server();

        Server(std::shared_ptr<Socket>);

        Server(uint16_t);

        virtual ~Server() {}

        void run_forever();

        void stop();

        void add_socket(std::shared_ptr<Socket>);

        void del_socket(std::shared_ptr<Socket>);

        void add_socket(Socket *);

        void del_socket(Socket *);

    };
}


#endif //CLAUSE_SERVER_SERVER_H
