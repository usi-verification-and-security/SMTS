//
// Author: Matteo Marescotti
//

#include <unistd.h>
#include <thread>
#include "lib/Logger.h"
#include "SolverServer.h"


SolverServer::SolverServer(const net::Address &server) :
        net::Server(),
        server(server),
        solver(nullptr) {
    net::Header header;
    header["solver"] = SolverProcess::solver;
    this->server.write(header, "");
    this->add_socket(&this->server);
}

SolverServer::~SolverServer() {
}

void SolverServer::log(log_level level, std::string message) {
    if (message.find("\n") != std::string::npos) {
        ::replace(message, "\n", "\n    ");
        message = "\n" + message;
    }
    Logger::log(level, message);
}

void SolverServer::handle_close(net::Socket &socket) {
    if (&socket == &this->server) {
        this->log(Logger::INFO, "server closed the connection");
        this->stop_solver();
    } else if (this->solver && &socket == this->solver->reader()) {
        this->log(Logger::ERROR, "unexpected solver quit");
        net::Header header;
        header["report"] = "error: unexpected solver quit";
        this->server.write(header, "");
        this->stop_solver();
    }
}

void SolverServer::handle_exception(net::Socket &socket, const std::exception &exception) {
    this->log(Logger::ERROR, exception.what());
}

void SolverServer::stop_solver() {
    if (!this->solver)
        return;
    this->log(Logger::INFO, "solver killed");
    this->del_socket(this->solver->reader());
    delete this->solver;
    this->solver = nullptr;
}

void SolverServer::update_lemmas() {
    if (!this->solver)
        return;
    net::Header header;
    header["command"] = "local";
    header["local"] = "lemma_server";
    header["lemma_server"] = this->lemmas_address;
    this->solver->writer()->write(header, "");
}


void SolverServer::handle_message(net::Socket &socket, net::Header &header, std::string &payload) {
    if (socket.get_fd() == this->server.get_fd()) {
        if (header.count("command") != 1) {
            this->log(Logger::WARNING, "unexpected message from server without command");
            return;
        }
        if (header["command"] == "lemmas" && header.count("lemmas") == 1) {
            this->lemmas_address = header["lemmas"];
            this->update_lemmas();
        } else if (header["command"] == "solve") {
            this->stop_solver();
            header.erase("command");
            this->solver = new SolverProcess(header, payload, this->lemmas_address.empty() ? false:true);
            this->update_lemmas();
            this->add_socket(this->solver->reader());
            this->log(Logger::INFO, "solver started: " + header["name"] + header["node"]);
        } else if (header["command"] == "stop") {
            this->stop_solver();
        }
        else {
            this->solver->writer()->write(header, payload);
            //this->solver->pipe.writer()->write(header, payload);
//            std::cout << "\033[1;51m [t Listener] \033[0m"<<"Header:"+ header["command"]
//                    <<"Header:"+ header["local"]
//                    <<"Header:"+ header["lemma_server"]
//            <<endl;
        }

    } else if (this->solver && &socket == this->solver->reader()) {
        this->server.write(header, payload);
        if (header.count("report")) {
            auto report = ::split(header["report"], ":", 2);
            log_level level = Logger::INFO;
            if (report.size() == 2) {
                if (report[0] == "error")
                    level = Logger::ERROR;
                else if (report[0] == "warning")
                    level = Logger::WARNING;
            }
            this->log(level, "solver report: " + report.back());
        }
    }
}
