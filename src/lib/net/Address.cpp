/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#include "Address.h"

#include <PTPLib/common/Exception.hpp>

#include <arpa/inet.h>

namespace net {

    Address::Address(std::string const & address) {
        uint8_t i;
        for (i = 0; address[i] != ':' and i < address.size() && i < (uint8_t) -1; i++) {
        }
        if (address[i] != ':')
            throw PTPLib::common::Exception(__FILE__, __LINE__, "invalid host:port");
        new(this) Address(address.substr(0, i), (uint16_t) ::atoi(address.substr(i + 1).c_str()));
    }

    Address::Address(std::string const & hostname, uint16_t port)
        : hostname(hostname)
        , port(port)
    {}

    Address::Address(struct sockaddr_storage * address) {
        char ipstr[INET6_ADDRSTRLEN];
        uint16_t port = 0;

        if (address->ss_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in *) address;
            port = ntohs(s->sin_port);
            ::inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
        } else { // AF_INET6
            struct sockaddr_in6 *s = (struct sockaddr_in6 *) address;
            port = ntohs(s->sin6_port);
            ::inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
        }

        new(this) Address(ipstr, port);
    }
}