//
// Created by Matteo on 12/08/16.
//

#ifndef CLAUSE_SERVER_SOCKET_H
#define CLAUSE_SERVER_SOCKET_H

#include <map>
#include <memory>
#include <mutex>
#include "lib/Exception.h"
#include "Address.h"
#include "Header.h"


namespace net {
    class SocketException : public Exception {
    public:
        explicit SocketException(const char *message) : SocketException(std::string(message)) {}

        explicit SocketException(const std::string &message) : Exception("SocketException: " + message) {}
    };


    class SocketClosedException : public SocketException {
    public:
        explicit SocketClosedException() : SocketException("file descriptor closed") {}
    };


    class Socket {
    private:
        int fd;
        mutable std::mutex read_mtx, write_mtx;

        inline uint32_t readn(char *, uint32_t) const;

    public:
        Socket(int);

        Socket(const Address);

        Socket(const std::string &);

        Socket(uint16_t);

        ~Socket();

        std::shared_ptr<Socket> accept() const;

        uint32_t read(net::Header &, std::string &) const;

        uint32_t write(const net::Header &, const std::string &) const;

        void close();

        int get_fd() const;

        Address get_local() const;

        Address get_remote() const;

    };
}


#endif //CLAUSE_SERVER_SOCKET_H
