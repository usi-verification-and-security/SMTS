/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#include "SolverServer.h"
#include "lib/net/Report.h"
#include "lib/Logger.h"

#include <iostream>

void ReportSolverType(net::Socket const &);

SolverServer::SolverServer(net::Address const & server)
: net::Server()
, SMTSServer_socket(server)
, thread_pool( __func__ ,  6)
, schedular(thread_pool, synced_stream, channel, SMTSServer_socket, log_enabled) {
    ReportSolverType(get_SMTS_server_socket());
    channel.setParallelMode();
    this->add_socket(&SMTSServer_socket);
}

void ReportSolverType(net::Socket const & SMTS_server)
{
    PTPLib::net::Header header;
    header[PTPLib::common::Param.SOLVER] = SolverProcess::solver;
    SMTS_server.write(PTPLib::net::SMTS_Event(std::move(header)));
}

void SolverServer::handle_event(net::Socket & socket, PTPLib::net::SMTS_Event && SMTS_event) {
    assert(not SMTS_event.empty());
    if (socket.get_fd() == this->SMTSServer_socket.get_fd()) {
        if (SMTS_event.header.count("enableLog") == 1 and SMTS_event.header.at("enableLog") == "1") {
            schedular.set_log_enabled(log_enabled = true);
            thread_pool.set_syncedStream(synced_stream);
        }

        if (SMTS_event.header.count("communication") == 1 and SMTS_event.header.at("communication") == "1") {
            schedular.push_to_pool(PTPLib::common::TASK::MEMORYCHECK,
                                   atoi(SMTS_event.header.at(PTPLib::common::Param.MAX_MEMORY).c_str()));
            schedular.push_to_pool(PTPLib::common::TASK::COMMUNICATION);
            return;
        }
        bool reset = false;
        assert(SMTS_event.header.count(PTPLib::common::Param.COMMAND));
        if (SMTS_event.header.count(PTPLib::common::Param.COMMAND) != 1) {
            if (log_enabled)
                synced_stream.println(log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                                      "unexpected message from server without command");
            return;
        }
        if (SMTS_event.header[PTPLib::common::Param.COMMAND] == PTPLib::common::Command.TERMINATE)
        {
            if (log_enabled)
                synced_stream.println(log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                                      "unexpected message from server to terminate");
            exit(EXIT_FAILURE);
        }
        else if (SMTS_event.header[PTPLib::common::Param.COMMAND] == PTPLib::common::Command.LEMMAS) {
            this->lemmaServer_address = SMTS_event.header[PTPLib::common::Command.LEMMAS];
            this->schedular.set_lemma_amount(SMTS_event.header[PTPLib::common::Param.LEMMA_AMOUNT]);
            this->initiate_lemma_server(SMTS_event);
            getChannel().setClauseShareMode();
        }
        else {
            if (SMTS_event.header[PTPLib::common::Param.COMMAND] == PTPLib::common::Command.SOLVE)
            {
                if (this->lemmaServer_socket)
                    this->push_lemma_workers(SMTS_event);
            }
            std::unique_lock<std::mutex> listener_lk(getChannel().getMutex());
            assert([&]() {
                if (not listener_lk.owns_lock()) {
                    throw PTPLib::common::Exception(__FILE__, __LINE__, std::string(__FUNCTION__) + " can't take the lock");
                }
                return true;
            }());
            if (SMTS_event.header[PTPLib::common::Param.COMMAND] == PTPLib::common::Command.STOP) {
                reset = true;
                if (not getChannel().isEmpty_query())
                    getChannel().clear_queries();
            }
            if (log_enabled)
                synced_stream.println_bold(log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                                           "[ t ", __func__, "] -> ", "Updating the queue with ",
                                           SMTS_event.header.at(PTPLib::common::Param.COMMAND), " and waiting");
            schedular.queue_event(std::move(SMTS_event));
            listener_lk.unlock();
        }
        if (reset)
            stop_schedular();

    } else if (&socket == &this->get_SMTS_server_socket()) {
        this->SMTSServer_socket.write(SMTS_event);
        if (SMTS_event.header.count("report")) {
            auto report = ::split(SMTS_event.header["report"], ":", 2);
            log_level level = Logger::INFO;
            if (report.size() == 2) {
                if (report[0] == "error")
                    level = Logger::ERROR;
                else if (report[0] == "warning")
                    level = Logger::WARNING;
            }
            if (log_enabled)
                synced_stream.println_bold(log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                                           "solver report: " + report.back());
        }
    }
}

void SolverServer::stop_schedular() {
    if (log_enabled)
        synced_stream.println(log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                              "solver Killed!");
    schedular.notify_reset();
    schedular.getPool().wait_for_tasks();
    assert(not schedular.getPool().get_tasks_total());
    {
        std::scoped_lock<std::mutex> _lk(getChannel().getMutex());
        getChannel().resetChannel();
    }
}

void SolverServer::initiate_lemma_server(PTPLib::net::SMTS_Event & SMTS_Event) {

    if (log_enabled)
        synced_stream.println(log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                              "LemmasServer Address: ", this->lemmaServer_address);
    if (not this->lemmaServer_address.empty()) {
        try {
            this->lemmaServer_socket.reset(new net::Socket(this->lemmaServer_address));
            schedular.set_LemmaServer_socket(this->lemmaServer_socket.get());
        } catch (net::SocketException &ex) {
            net::Report::error(get_SMTS_server_socket(), SMTS_Event.header, std::string("lemma server connection failed: ") + ex.what());
        }
    }
}

void SolverServer::push_lemma_workers(PTPLib::net::SMTS_Event & SMTS_event)
{
    int seed = atoi(SMTS_event.header.get(PTPLib::net::parameter, PTPLib::common::Param.SEED).c_str());
    int interval = atoi(SMTS_event.header["lemma_push_min"].c_str()) + seed
            % ( atoi(SMTS_event.header["lemma_push_max"].c_str()) - atoi(SMTS_event.header["lemma_push_min"].c_str()) + 1 );

    schedular.push_to_pool(PTPLib::common::TASK::CLAUSEPUSH, seed, atoi(SMTS_event.header["lemma_push_min"].c_str()),
                           atoi(SMTS_event.header["lemma_push_max"].c_str()));
    schedular.push_to_pool(PTPLib::common::TASK::CLAUSEPULL, seed, atoi(SMTS_event.header["lemma_pull_min"].c_str()),
                           atoi(SMTS_event.header["lemma_pull_max"].c_str()));
    schedular.push_to_pool(PTPLib::common::TASK::CLAUSELEARN, interval / 2);
}

void SolverServer::handle_close(net::Socket & socket) {
    if (log_enabled)
        synced_stream.println(log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                              "server closed the connection");
    exit(EXIT_SUCCESS);
}

void SolverServer::handle_exception(net::Socket const & socket, const std::exception & exception) {
    synced_stream.println(log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT, exception.what());
}