//
// Author: Matteo Marescotti
//

#ifndef SMTS_LIB_NET_SOCKET_H
#define SMTS_LIB_NET_SOCKET_H

#include <map>
#include <memory>
#include <mutex>
#include "lib/Exception.h"
#include "Address.h"
#include "Header.h"


namespace net {
    class SocketException : public Exception {
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


#endif
