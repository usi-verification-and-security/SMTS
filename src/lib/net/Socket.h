/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMTS_LIB_NET_SOCKET_H
#define SMTS_LIB_NET_SOCKET_H

#include "Address.h"

#include "PTPLib/common/Exception.hpp"
#include "PTPLib/net/SMTSEvent.hpp"

#include <map>
#include <memory>
#include <mutex>

namespace net {
    class SocketException : public PTPLib::common::Exception {
    public:
        explicit SocketException(const char *file, unsigned line, const std::string &message) :
                Exception(file, line, "SocketException: " + message) {}
    };

    class SocketClosedException : public SocketException {
    public:
        explicit SocketClosedException(const char *file, unsigned line) :
                SocketException(file, line, "file descriptor closed") {}
    };

    class Socket {
    private:
        int fd;
        mutable std::mutex read_mtx, write_mtx;

        inline uint32_t readn(char *, uint32_t) const;
        int Id = 0;

    public:
        Socket(int);

        Socket(const Address);

        Socket(const std::string &);

        Socket(uint16_t);

        ~Socket();

        std::shared_ptr<Socket> accept() const;

        PTPLib::net::SMTS_Event read(uint32_t & length) const;

        uint32_t write(PTPLib::net::SMTS_Event const &) const;

        void close();

        int get_fd() const;

        Address get_local() const;

        Address get_remote() const;

        void setId(int id) { Id = id; }

        int getId() const { return Id; }
    };
}


#endif
