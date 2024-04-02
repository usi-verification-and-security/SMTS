/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMTS_LIB_NET_ADDRESS_H
#define SMTS_LIB_NET_ADDRESS_H

#include <string>
#include <sstream>
#include <cstdint>
#include <sys/socket.h>

namespace net {

    class Address {

    public:
        Address(std::string const & address);

        Address(std::string const & host_name, uint16_t port);

        Address(struct sockaddr_storage * sockaddrStorage);

        friend std::ostream &operator<<(std::ostream & stream, Address const & address) {
            return stream << address.hostname << ":" << std::to_string(address.port);
        }

        std::string hostname;
        uint16_t port;
    };
}


#endif
