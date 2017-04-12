//
// Created by Matteo on 22/07/16.
//

#include <unistd.h>
#include <thread>
#include <chrono>
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

void SolverServer::log(uint8_t level, std::string message) {
    if (message.find("\n") != std::string::npos) {
        ::replace(message, "\n", "\n    ");
        message = "\n" + message;
    }
    Logger::log(level, (this->solver ? this->solver->header["name"] + this->solver->header["node"] + ": " : "") +
                       message);
}


bool SolverServer::check_header(net::Header &header) {
    if (this->solver == nullptr)
        return false;
    return header["name"] == this->solver->header["name"] && header["node"] == this->solver->header["node"];
}


void SolverServer::handle_close(net::Socket &socket) {
    if (&socket == &this->server) {
        this->log(Logger::INFO, "server closed the connection");
        this->stop_solver();
    } else if (this->solver && &socket == this->solver->reader()) {
        this->log(Logger::ERROR, "solver quit unexpected");
        this->solver->header["report"] = "unknown";
        this->server.write(this->solver->header, "");
        this->stop_solver();
    }
}

void SolverServer::handle_exception(net::Socket &socket, const net::SocketException &exception) {
    this->log(Logger::ERROR, exception.what());
}

void SolverServer::stop_solver() {
    if (!this->solver)
        return;
    this->log(Logger::INFO, "solver stop");
    this->del_socket(this->solver->reader());
    delete this->solver;
    this->solver = nullptr;
}

void SolverServer::update_lemmas() {
    if (!this->solver)
        return;
    auto header = this->solver->header;
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
            if (this->check_header(header)) {
                return;
            }
            this->stop_solver();
            header.erase("command");
            this->solver = new SolverProcess(header, payload);
            this->update_lemmas();
            this->add_socket(this->solver->reader());
        } else if (header["command"] == "stop") {
            if (!this->check_header(header)) {
                return;
            }
            this->stop_solver();
        } else if (this->check_header(header)) {
            this->solver->writer()->write(header, payload);
        }
    } else if (this->solver && &socket == this->solver->reader()) {
        this->server.write(header, payload);
        this->solver->header = header;
        if (header.count("report")) {
            auto report = ::split(header["report"], ":", 2);
            uint8_t level = Logger::INFO;
            if (report->size() == 2) {
                if (report->at(0) == "error")
                    level = Logger::ERROR;
                else if (report->at(0) == "warning")
                    level = Logger::WARNING;
            }
            this->log(level, report->back());
            this->solver->header.erase("report");
        }
    }
}
