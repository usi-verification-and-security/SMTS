/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#include "Socket.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <algorithm>
#include <string.h>


namespace net {
    Socket::Socket(const Address address) {
        int sockfd;
        struct sockaddr_in server_addr;
        struct hostent *he;

        if ((sockfd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0)
            throw SocketException(__FILE__, __LINE__, "socket init error");

        if ((he = ::gethostbyname(address.hostname.c_str())) == nullptr)
            throw SocketException(__FILE__, __LINE__, "invalid hostname");

        ::bzero(&server_addr, sizeof(server_addr));
        ::memcpy(&server_addr.sin_addr, he->h_addr_list[0], (size_t) he->h_length);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(address.port);

        if (::connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0)
            throw SocketException(__FILE__, __LINE__, "connect error");

        new(this) Socket(sockfd);
    }

    Socket::Socket(const std::string &address) : Socket(Address(address)) {}

    Socket::Socket(uint16_t port) {
        int sockfd;
        struct sockaddr_in server_addr;

        if ((sockfd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0)
            throw SocketException(__FILE__, __LINE__, "socket init failed");

        int reuse = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
            throw SocketException(__FILE__, __LINE__, "socket SO_REUSEADDR failed");

        ::bzero(&server_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (::bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0)
            throw SocketException(__FILE__, __LINE__, "bind failed");

        if (::listen(sockfd, 1024) != 0)
            throw SocketException(__FILE__, __LINE__, "listen error");

        new(this) Socket(sockfd);
    }

    Socket::Socket(int fd) : fd(fd) {};

    Socket::~Socket() {
        this->close();
    }

    std::shared_ptr<Socket> Socket::accept() const {
        int clientfd;
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        if ((clientfd = ::accept(this->fd, (struct sockaddr *) &client_addr, &addr_len)) < 0)
            throw SocketException(__FILE__, __LINE__, "accept error");

        return std::shared_ptr<Socket>(new Socket(clientfd));
    }

    uint32_t Socket::readn(char *buffer, uint32_t length) const {
        uint32_t r = 0;
        while (length > r) {
            ssize_t t = ::read(this->fd, &buffer[r], length - r);
            if (t < 0) {
                if (errno == ECONNRESET)
                    t = 0;
                else
                    throw SocketException(__FILE__, __LINE__, strerror(errno));
            }
            if (t == 0)
                throw SocketClosedException(__FILE__, __LINE__);
            r += t;
        }
        return r;
    }

    PTPLib::net::SMTS_Event Socket::read(uint32_t & length) const {
        std::scoped_lock<std::mutex> _l(this->read_mtx);
        PTPLib::net::Header header;
        std::string payload;
        char buffer[4];
        if (this->readn(buffer, 4) != 4)
            return PTPLib::net::SMTS_Event();
        length = (uint32_t) ((uint8_t) buffer[0]) << 24 |
                 (uint32_t) ((uint8_t) buffer[1]) << 16 |
                 (uint32_t) ((uint8_t) buffer[2]) << 8 |
                 (uint32_t) ((uint8_t) buffer[3]);
        std::unique_ptr<char> message((char *) malloc(length));
        if (message == nullptr)
            throw SocketException(__FILE__, __LINE__, "can't malloc");

        length = this->readn(message.get(), length);

        uint32_t i = 0;
        header.clear();
        while (message.get()[i] != '\x00') {
            std::string keyval[2] = {"", ""};
            for (uint8_t j = 0; j < 2; j++) {
                uint8_t l = (uint8_t) message.get()[i++];
                if (i + l >= length)
                    throw SocketException(__FILE__, __LINE__, "error during header parsing");
                keyval[j] += std::string(&message.get()[i], l);
                i += l;
            }
            header[keyval[0]] = keyval[1];
        }
        i++;

        if (length > i)
            payload.append(std::string(&message.get()[i], length - i));
        return PTPLib::net::SMTS_Event(std::move(header), std::move(payload));
    }

    uint32_t Socket::write(const PTPLib::net::SMTS_Event & SMTS_Event) const {
        std::scoped_lock<std::mutex> _l(this->write_mtx);
        if (SMTS_Event.header.count(""))
            throw SocketException(__FILE__, __LINE__, "empty key is not allowed");
        std::string message;
        message += "\xFF\xFF\xFF\xFF";
        for (auto &pair : SMTS_Event.header) {
            std::string keyval[2] = {pair.first, pair.second};
            for (uint8_t i = 0; i < 2; i++) {
                if (keyval[i].length() > (uint8_t) -1)
                    throw SocketException(__FILE__, __LINE__, "header key or value is too big");
                message += (char) keyval[i].length();
                message += keyval[i];
            }
        }

        message += '\x00';
        message += SMTS_Event.body;

        if (message.length() > (uint32_t) -1)
            throw SocketException(__FILE__, __LINE__, "resulting message is too big");
        uint32_t length = (uint32_t) message.length() - 4;
        message[3] = (char) length;
        message[2] = (char) (length >> 8);
        message[1] = (char) (length >> 16);
        message[0] = (char) (length >> 24);
        if (::write(this->fd, message.c_str(), message.size()) != (ssize_t) message.size())
            throw SocketException(__FILE__, __LINE__, "write error");

        return length;
    }

    void Socket::close() {
        ::close(this->fd);
        this->fd = -1;
    }

    int Socket::get_fd() const {
        return this->fd;
    }

    Address Socket::get_local() const {
        struct sockaddr_storage addr;
        socklen_t addr_len = sizeof(addr);

        if (::getsockname(this->fd, (struct sockaddr *) &addr, &addr_len) < 0)
            return Address("", 0);

        return Address(&addr);
    }

    Address Socket::get_remote() const {
        struct sockaddr_storage addr;
        socklen_t addr_len = sizeof(addr);

        if (::getpeername(this->fd, (struct sockaddr *) &addr, &addr_len) < 0)
            return Address("", 0);

        return Address(&addr);
    }
}