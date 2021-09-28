//
// Author: Matteo Marescotti
//

#include <thread>
#include "lib/Logger.h"
#include "SolverServer.h"
#include <iostream>
#include <string.h>
#include <signal.h>

SolverServer::SolverServer(const net::Address &server) :
        net::Server(),
        server(server),
        lemmas_address(""),
        solver(nullptr)
        {
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
    try {
        if (!this->solver)
            return;
        this->log(Logger::INFO, "solver killed");
        this->solver->interrupt("stop");
//         std::cout <<"before this->solver->getChannel().notify_one(); "<<endl;
        this->solver->cv.notify_one();
//        std::cout <<"after this->solver->getChannel().notify_one(); "<<endl;
        delete this->solver;
        this->solver = nullptr;
    }
    catch (...) {
        std::cout << "exception stop_solver " << std::endl;
    }
}

void SolverServer::update_lemmas() {
    if (!this->solver)
        return;
    net::Header header;
    header["command"] = "local";
    header["local"] = "lemma_server";
    header["lemma_server"] = this->lemmas_address;
    if (!this->lemmas_address.size()) {
//        std::lock_guard<std::mutex> _l(this->lemma.mtx);
        this->solver->lemma.server.reset();
    } else {
        try {
//            std::lock_guard<std::mutex> _l(this->lemma.mtx);
            this->solver->lemma.server.reset(new net::Socket(this->lemmas_address));
            this->solver->lemma.errors = 0;
        } catch (net::SocketException &ex) {
//            this->solver->error(std::string("lemma server connection failed: ") + ex.what());
//            continue;
        }
//        this->lemma_push(std::vector<net::Lemma>());
    }
    this->solver->writer()->write(header, "");
}

static void segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
//    printf("Caught segfault at address %p\n", si->si_addr);
//        exit(0);
}
//static int lemmaPush_timeout_min;
//int getSeed()
//{
//    return lemmaPush_timeout_min;
//}
void SolverServer::handle_message(net::Socket &socket, net::Header &header, std::string &payload) {
//    this->solver->getChannel()
    if (socket.get_fd() == this->server.get_fd()) {
        if (header.count("command") != 1) {
            this->log(Logger::WARNING, "unexpected message from server without command");
            return;
        }
        if (header["command"] == "lemmas" && header.count("lemmas") == 1) {
            this->lemmas_address = header["lemmas"];

            this->update_lemmas();
        } else if (header["command"] == "solve") {
//            struct sigaction sa;
//
//            memset(&sa, 0, sizeof(struct sigaction));
//            sigemptyset(&sa.sa_mask);
//            sa.sa_sigaction = segfault_sigaction;
//            sa.sa_flags   = SA_SIGINFO;
//
//            sigaction(SIGSEGV, &sa, NULL);
            this->stop_solver();
            header.erase("command");

            this->solver = new SolverProcess(header, payload, &this->server, this->lemmas_address.empty() ? false : true);
            this->solver->lemmaPush_timeout_min = atoi(header["lemma_push_min"].c_str());
            this->solver->lemmaPull_timeout_min = atoi(header["lemma_pull_min"].c_str());
            this->solver->lemmaPush_timeout_max = atoi(header["lemma_push_max"].c_str());
            this->solver->lemmaPull_timeout_max = atoi(header["lemma_pull_max"].c_str());
            this->update_lemmas();
//            this->add_socket(this->solver->reader());
            this->log(Logger::INFO, "solver started: " + header["name"] + header["node"]);
        } else if (header["command"] == "stop") {
            this->stop_solver();
        }
        else {
//            std::cout << "\033[1;51m [t Listener\t] \033[0m"<<endl<<"Header: "+ header["command"]<<endl;
            this->solver->interrupt(header["command"]);

            std::unique_lock<std::mutex> lock(this->solver->mtx_listener_solve);
//            std::cout <<"incremental payload: "<<payload<<endl;
            this->solver->instance_Temp.push_back(payload);
//            std::unique_lock<std::mutex> lock(this->solver->stop_mutex);
            this->solver->header_Temp.push_back(header);

            lock.unlock();
            this->solver->cv.notify_one();
//            this->solver->writer()->write(header, payload);
//            this->solver->pipe.writer()->write(header, payload);


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
