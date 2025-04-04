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
, thread_pool( __func__ , 1)
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
        if (SMTS_event.header.count(PTPLib::common::Param.LOG_MODE) == 1 and ::to_bool(SMTS_event.header.at(PTPLib::common::Param.LOG_MODE))) {
            schedular.set_log_enabled(log_enabled = true);
            thread_pool.set_syncedStream(synced_stream);
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
            exit(EXIT_SUCCESS);
        }
        else if (SMTS_event.header[PTPLib::common::Param.COMMAND] == PTPLib::common::Command.LEMMAS) {
            this->schedular.set_lemma_amount(SMTS_event.header[PTPLib::common::Param.LEMMA_AMOUNT]);
            this->lemma_stat.lemma_push_min = atoi(SMTS_event.header[PTPLib::common::Param.L_PUSH_MIN].c_str());
            this->lemma_stat.lemma_push_max = atoi(SMTS_event.header[PTPLib::common::Param.L_PUSH_Max].c_str());
            this->lemma_stat.lemma_pull_min = atoi(SMTS_event.header[PTPLib::common::Param.L_PULL_MIN].c_str());
            this->lemma_stat.lemma_pull_max = atoi(SMTS_event.header[PTPLib::common::Param.L_PULL_MAX].c_str());
            this->initiate_lemma_server(SMTS_event);
            getChannel().setClauseShareMode();
            if (this->lemma_stat.seed) {
                thread_pool.increase(3);
                this->push_lemma_workers();
            }
        }
        else {
            std::unique_lock<std::mutex> listener_lk(getChannel().getMutex());
            if (SMTS_event.header[PTPLib::common::Param.COMMAND] == PTPLib::common::Command.SOLVE)
            {
                if (not log_enabled) {
                    FILE * file = fopen("/dev/null", "w");
                    dup2(fileno(file), fileno(stdout));
                    dup2(fileno(file), fileno(stderr));
                    fclose(file);
                }
                this->lemma_stat.seed = atoi(SMTS_event.header.get(PTPLib::net::parameter, PTPLib::common::Param.SEED).c_str());
                assert([&]() {
                    if (not listener_lk.owns_lock()) {
                        throw PTPLib::common::Exception(__FILE__, __LINE__, std::string(__FUNCTION__) + " can't take the lock");
                    }
                    return true;
                }());
                if (log_enabled)
                    synced_stream.println_bold(log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                                               "[ t ", __func__, "] -> ", "Updating the queue with ",
                                               SMTS_event.header.at(PTPLib::common::Param.COMMAND), " and waiting");

                if (schedular.preProcess_instance(SMTS_event))
                    setUpThreadArch(SMTS_event);
            }
            reset = schedular.queue_event(std::move(SMTS_event));
            listener_lk.unlock();
        }
        if (reset) {
            exit(EXIT_SUCCESS);
            stop_schedular();
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
                              "LemmasServer Address: ", SMTS_Event.header[PTPLib::common::Command.LEMMAS]);
    if (not SMTS_Event.header[PTPLib::common::Command.LEMMAS].empty() and not this->lemmaServer_socket) {
        try {
            this->lemmaServer_socket.reset(new net::Socket(SMTS_Event.header[PTPLib::common::Command.LEMMAS]));
            schedular.set_LemmaServer_socket(this->lemmaServer_socket.get());
        } catch (net::SocketException &ex) {
            net::Report::error(get_SMTS_server_socket(), SMTS_Event.header, std::string("lemma server connection failed: ") + ex.what());
        }
    }
}

void SolverServer::push_lemma_workers()
{
//    int interval = this->lemma_stat.lemma_push_min + this->lemma_stat.seed % ( this->lemma_stat.lemma_push_max - this->lemma_stat.lemma_push_min + 1 );

    schedular.push_to_pool(PTPLib::common::TASK::CLAUSEPUSH, this->lemma_stat.seed, this->lemma_stat.lemma_push_min, this->lemma_stat.lemma_push_max);
    schedular.push_to_pool(PTPLib::common::TASK::CLAUSEPULL, this->lemma_stat.seed, this->lemma_stat.lemma_pull_min, this->lemma_stat.lemma_pull_max);
    schedular.push_to_pool(PTPLib::common::TASK::CLAUSELEARN, 15000);
}

void SolverServer::handle_close(net::Socket & client) {
    (void) client;
    if (log_enabled)
        synced_stream.println(log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                              "server closed the connection");
    exit(EXIT_SUCCESS);
}

void SolverServer::handle_exception(net::Socket const & client, const std::exception & exception) {
    synced_stream.println(log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT, "Exception from: " + to_string(client.get_remote()) + ": ", exception.what());
}

void SolverServer::setUpThreadArch(PTPLib::net::SMTS_Event const & SMTS_event)
{
    if (this->lemma_stat.seed and this->lemmaServer_socket) {
        thread_pool.increase(3);
        this->push_lemma_workers();
    }
    if (SMTS_event.header.count(PTPLib::common::Param.MAX_MEMORY) == 1 and SMTS_event.header.at(PTPLib::common::Param.MAX_MEMORY) != "0") {
        thread_pool.increase(1);
        schedular.push_to_pool(PTPLib::common::TASK::MEMORYCHECK,
                               atoi(SMTS_event.header.at(PTPLib::common::Param.MAX_MEMORY).c_str()));
    }
    thread_pool.increase(1);
    schedular.push_to_pool(PTPLib::common::TASK::COMMUNICATION);
}