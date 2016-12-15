//
// Created by Matteo on 12/08/16.
//

#include <unistd.h>
#include <algorithm>
#include "Server.h"


namespace net {
    Server::Server(std::shared_ptr<Socket> socket) :
            socket(socket) {
        if (socket)
            this->sockets.insert(socket);
    }

    Server::Server() : Server(nullptr) {}

    Server::Server(uint16_t port) :
            Server(std::shared_ptr<Socket>(new Socket(port))) {}

    void Server::run_forever() {
        fd_set readset;
        int result;
        net::Header header;
        std::string payload;

        while (true) {
            do {
                FD_ZERO(&readset);
                int max = 0;
                for (auto &socket : this->sockets) {
                    if (socket->get_fd() < 0)
                        continue;
                    max = max < socket->get_fd() ? socket->get_fd() : max;
                    FD_SET(socket->get_fd(), &readset);
                }
                if (max == 0)
                    return;
                result = ::select(max + 1, &readset, nullptr, nullptr, nullptr);
            } while (result == -1 && errno == EINTR);

            auto socket = this->sockets.begin();
            while (socket != this->sockets.end()) {
                if ((*socket)->get_fd() < 0) {
                    this->del_socket(*socket);
                } else if (FD_ISSET((*socket)->get_fd(), &readset)) {
                    FD_CLR((*socket)->get_fd(), &readset);
                    if (this->socket && (*socket)->get_fd() == this->socket->get_fd()) {
                        std::shared_ptr<Socket> client;
                        try {
                            client = this->socket->accept();
                        }
                        catch (SocketException &ex) {
                            socket++;
                            continue;
                        }
                        this->sockets.insert(client);
                        this->handle_accept(*client);
                    } else {
                        try {
                            (*socket)->read(header, payload);
                            this->handle_message(**socket, header, payload);
                        }
                        catch (SocketClosedException &ex) {
                            std::shared_ptr<net::Socket> s = *socket;
                            this->sockets.erase(socket);
                            this->handle_close(*s);
                        }
                        catch (SocketException &ex) {
                            this->handle_exception(**socket, ex);
                        }
                        catch (std::exception &ex) {
                            this->handle_exception(**socket, SocketException(ex.what()));
                        }
                    }
                } else {
                    ++socket;
                    continue;
                }
                socket = this->sockets.begin();
            }
//        if (result < 0) {
//        }
        }
    }

    void Server::stop() {
        this->sockets.clear();
    }

    void Server::add_socket(std::shared_ptr<Socket> socket) {
        if (this->sockets.find(socket) == this->sockets.end())
            this->sockets.insert(socket);
    }

    void Server::del_socket(std::shared_ptr<Socket> socket) {
        auto it = this->sockets.find(socket);
        if (it != this->sockets.end())
            this->sockets.erase(it);
    }

    void Server::add_socket(Socket *socket) {
        this->add_socket(std::shared_ptr<Socket>(socket, [](Socket *) {}));
    }

    void Server::del_socket(Socket *socket) {
        this->del_socket(std::shared_ptr<Socket>(socket, [](Socket *) {}));
    }
}