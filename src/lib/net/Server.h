//
// Author: Matteo Marescotti
//

#ifndef SMTS_LIB_NET_SERVER_H
#define SMTS_LIB_NET_SERVER_H

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

        virtual void handle_exception(Socket &, const std::exception &) {}

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


#endif
