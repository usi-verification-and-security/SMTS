/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMTS_LIB_NET_SERVER_H
#define SMTS_LIB_NET_SERVER_H

#include "Socket.h"

#include <list>
#include <set>

namespace net {

    class Server {
    private:
        std::shared_ptr<Socket> socket;
        std::set<std::shared_ptr<Socket>> sockets;

    protected:
        virtual void handle_accept(Socket const &) {}

        virtual void handle_close(Socket &) {}

        virtual void handle_event(Socket &, PTPLib::net::SMTS_Event &&) {}

        virtual void handle_exception(Socket const &, const std::exception &) {}

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
