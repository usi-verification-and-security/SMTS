//
// Author: Matteo Marescotti
//

#include <unistd.h>
#include <algorithm>
#include "Server.h"
#include <thread>
#include "lib/Logger.h"
#include "lib/Thread_pool.hpp"

Server::Server(std::shared_ptr<net::Socket> socket) :
        socket(socket) {
    if (socket)
        this->sockets.insert(socket);
}

Server::Server() : Server(nullptr) {}

Server::Server(uint16_t port) :
        Server(std::shared_ptr<net::Socket>(new net::Socket(port))) {}

void Server::run_forever() {
    fd_set readset;
    int result;

//    std::uint_fast32_t poolSize = std::thread::hardware_concurrency()-2;
//    const std::uint_fast32_t thread_count = poolSize;
//    thread_pool pool(poolSize);
    while (true) {
        do {
            std::cout << "[LemmServer] SSelect "<< std::endl;
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
            std::cout << "[LemmServer] ESelect "<< std::endl;
        } while (result == -1 && errno == EINTR);

        auto socket = this->sockets.begin();
        while (socket != this->sockets.end())
        {
            if ((*socket)->get_fd() < 0) {
                this->del_socket(*socket);
            }
            else if (FD_ISSET((*socket)->get_fd(), &readset)) {
                FD_CLR((*socket)->get_fd(), &readset);
                if (this->socket && (*socket)->get_fd() == this->socket->get_fd()) {
                    std::shared_ptr<net::Socket> client;
                    try {
                        client = this->socket->accept();
                    }
                    catch (net::SocketException &ex) {
                        socket++;
                        continue;
                    }
                    this->sockets.insert(client);
                    this->handle_accept(*client);

                }
                else {

//                        pool.push_task([this, &socket] {
                            try {
                                net::Header header;
                                std::string payload;
                                std::cout << "[LemmServer] SRead "<< std::endl;
                                (*socket)->read(header, payload);
                                std::cout << "[LemmServer] ERead for node -> "+header["node"]<< std::endl;
                                this->handle_message(**socket, header, payload);
                            }
                            catch (net::SocketClosedException &ex) {
//                            pool.reset();
                            std::shared_ptr<net::Socket> s = *socket;
                            this->sockets.erase(socket);
                            this->handle_close(*s);
                        }
                            catch (net::SocketException &ex) {
                            this->handle_exception(**socket, ex);
                        }
                            catch (std::exception &ex) {
                            this->handle_exception(**socket, ex);
                        }
//                        });
//                        if (poolSize == pool.get_tasks_total())
//                        {
//                            pool.wait_for_tasks();
//                            //poolSize = thread_count;
//                        }
                }
            }
            else{
                ++socket;
                continue;
            }
            socket = this->sockets.begin();
        }
    }
//    pool.wait_for_tasks();
}
void Server::task(net::Socket& socket, Server& s)
{
    net::Header header;
    std::string payload;
    socket.read(header, payload);
    std::cout << "\033[1;51m [t Listener] Read socket\033[0m"<<endl;
#ifdef ENABLE_DEBUGING
    if(header["command"].size()!=0){
        Logger::writeIntoFile(true,payload,"Recieved command: "+header["command"],getpid());

    }
#endif
    s.handle_message(socket, header, payload);
}
void Server::stop() {
    this->sockets.clear();
}

void Server::add_socket(std::shared_ptr<net::Socket> socket) {
    if (this->sockets.find(socket) == this->sockets.end())
        this->sockets.insert(socket);
}

void Server::del_socket(std::shared_ptr<net::Socket> socket) {
    auto it = this->sockets.find(socket);
    if (it != this->sockets.end())
        this->sockets.erase(it);
}

void Server::add_socket(net::Socket *socket) {
    this->add_socket(std::shared_ptr<net::Socket>(socket, [](net::Socket *) {}));
}

void Server::del_socket(net::Socket *socket) {
    this->del_socket(std::shared_ptr<net::Socket>(socket, [](net::Socket *) {}));
}
