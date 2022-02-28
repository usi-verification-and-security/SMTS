//
// Author: Matteo Marescotti
//

#include <thread>
#include "lib/Logger.h"
#include "SolverServer.h"
#include <iostream>
#include <signal.h>

SolverServer::SolverServer(const net::Address &server) :
        net::Server(),
        SMTSServer(server),
        solver(nullptr)
{
    net::Header header;
    header["solver"] = SolverProcess::solver;
    this->SMTSServer.write(header, "");
    this->add_socket(&this->SMTSServer);
}

SolverServer::~SolverServer() {
    free(solver);
}

void SolverServer::log(log_level level, std::string message) {
#ifdef ENABLE_DEBUGING
    if (message.find("\n") != std::string::npos) {
        ::replace(message, "\n", "\n    ");
        message = "\n" + message;
    }
    Logger::log(level, message);
#endif
}
static void segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
    printf("Caught segfault at address %p\n", si->si_addr);
    exit(0);
}
void SolverServer::handle_close(net::Socket &socket) {
//    struct sigaction sa;
//    memset(&sa, 0, sizeof(struct sigaction));
//    sigemptyset(&sa.sa_mask);
//    sa.sa_sigaction = segfault_sigaction;
//    sa.sa_flags   = SA_SIGINFO;
//    sigaction(SIGSEGV, &sa, NULL);
    if (&socket == &this->SMTSServer) {
        this->log(Logger::INFO, "server closed the connection");
        if (this->solver)
            if (this->solver->forked)
                this->solver->kill_child();
        exit(0);
//        this->stop_solver();
    } else if (this->solver && &socket == this->solver->reader()) {
        this->log(Logger::ERROR, "unexpected solver quit");
        net::Header header;
        header["report"] = "error: unexpected solver quit";
        this->SMTSServer.write(header, "");
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
    this->solver->interrupt(PartitionChannel::Command.Stop);
    this->solver->getChannel().notify_all();
    delete this->solver;
    this->solver = nullptr;
    std::scoped_lock<std::mutex> lk(channel.getMutex());
    channel.resetChannel();
}

void SolverServer::update_lemmas() {
    if (not this->solver)
        return;
//    if (not this->solver->lemma.server)
//        this->solver->lemma.server.reset(new net::Socket(this->lemmaServerAddress));
//    return;
    net::Header header;
//    header["command"] = "local";
//    header["local"] = "lemma_server";
    header["lemma_server"] = this->lemmaServerAddress;
//    std::scoped_lock<std::mutex> _l(this->solver->mtx_listener_solve);
    if (this->lemmaServerAddress.empty()) {
        this->solver->lemma.server.reset();
    }
    else {
        try {
            this->solver->lemma.server.reset(new net::Socket(this->lemmaServerAddress));
            this->solver->lemma.errors = 0;
        } catch (net::SocketException &ex) {
            this->solver->error(std::string("lemma server connection failed: ") + ex.what());
        }
//        this->solver->injectPulledClauses();
    }
//    this->solver->writer()->write(header, "");
}


void SolverServer::handle_message(net::Socket &socket, net::Header &header, std::string &payload) {

    if (socket.get_fd() == this->SMTSServer.get_fd()) {
        if (header.count("command") != 1) {
            this->log(Logger::WARNING, "unexpected message from server without command");
            return;
        }
        if (header["command"] == PartitionChannel::Command.Terminate)
        {
            if (this->solver)
                if (this->solver->forked)
                    this->solver->kill_child();
            this->log(Logger::WARNING, "unexpected message from server to terminate");
            exit(0);
        }
        else if (header["command"] == PartitionChannel::Command.Lemmas && header.count(PartitionChannel::Command.Lemmas) == 1) {
            this->lemmaServerAddress = header[PartitionChannel::Command.Lemmas];

            this->update_lemmas();
            if (this->solver)
                this->solver->start_lemma_threads();
        }
        else if (header["command"] == PartitionChannel::Command.Solve) {
//            SMTS::colorMode = static_cast<bool>(atoi(header["colorMode"].c_str()));
            this->stop_solver();
            header.erase("command");
            this->solver = new SolverProcess(header, payload, &this->SMTSServer, channel,
                                             this->lemmaServerAddress.empty() ? false : true);


            this->update_lemmas();
            this->log(Logger::INFO, "solver started: " + header["name"] + header["node"]);
        }
        else if (header["command"] == PartitionChannel::Command.Stop)
        {
            this->stop_solver();
        }
        else {
            std::unique_lock<std::mutex> lk(this->solver->getChannel().getMutex());
//            std::unique_lock<std::mutex> lock(this->solver->mtx_listener_solve);
            this->solver->instance_Temp.push_back(payload);
            this->solver->header_Temp.push_back(header);
            this->solver->interrupt(header["command"]);
            lk.unlock();
            this->solver->getChannel().notify_all();
        }
    } else if (this->solver && &socket == this->solver->reader()) {
        this->SMTSServer.write(header, payload);
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
