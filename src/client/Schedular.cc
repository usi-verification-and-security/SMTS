/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#include "Schedular.h"
#include "lib/net/Report.h"

#include <PTPLib/common/Memory.hpp>
#include "PTPLib/net/Lemma.hpp"
#include <PTPLib/common/Exception.hpp>

#include <string>
#include <cmath>
#include <cassert>

void Schedular::memory_checker(int max_memory)
{
#ifdef VERBOSE_THREAD
    memory_thread_id = std::this_thread::get_id();
#endif
    size_t limit = static_cast<std::size_t>(max_memory);
    if (limit == 0)
        return;

    while (true) {
        size_t memory_size_b = PTPLib::common::current_memory();
        if (memory_size_b > limit * 1024 * 1024) {
            std::scoped_lock<std::mutex> lk(channel.getMutex());
            auto current_header = channel.get_current_header({PTPLib::common::Param.NAME, PTPLib::common::Param.NODE});
            net::Report::error(get_SMTS_server(), current_header, " max memory reached: " + std::to_string(memory_size_b));
            exit(EXIT_FAILURE);
        }
        if (log_enabled)
            synced_stream.println(log_enabled ? PTPLib::common::Color::FG_Yellow : PTPLib::common::Color::FG_DEFAULT, "[ t ", __func__, "] -> "
                       , std::to_string(memory_size_b));
        std::unique_lock<std::mutex> lk(channel.getMutex());
        if (channel.wait_for_reset(lk, std::chrono::seconds (5)))
            break;
#ifdef VERBOSE_THREAD
            if (memory_thread_id != std::this_thread::get_id())
                throw PTPLib::common::Exception(__FILE__, __LINE__, std::string(__FUNCTION__) +" has inconsistent thread id");
#endif

        assert([&]() {
            if (not lk.owns_lock()) {
                throw PTPLib::common::Exception(__FILE__, __LINE__, std::string(__FUNCTION__) + " can't take the lock");
            }
            return true;
        }());
    }
}

void Schedular::push_to_pool(PTPLib::common::TASK t_name, int seed, int td_min, int td_max) {
    thread_pool.push_task([this, t_name, seed, td_min, td_max] {
        worker(t_name, seed, td_min, td_max);
    }, ::get_task_name(t_name));
}

void Schedular::worker(PTPLib::common::TASK wname, int seed, int td_min, int td_max) {

    switch (wname) {

        case PTPLib::common::TASK::COMMUNICATION:
            communicate_worker();
            break;

        case PTPLib::common::TASK::CLAUSEPUSH:
            push_clause_worker(seed, td_min, td_max);
            break;

        case PTPLib::common::TASK::CLAUSEPULL:
            pull_clause_worker(seed, td_min, td_max);
            break;

        case PTPLib::common::TASK::MEMORYCHECK:
            memory_checker(seed);
            break;

        case PTPLib::common::TASK::CLAUSELEARN:
            periodic_clauseLearning_worker(seed);
            break;

        default:
            break;
    }

}

void Schedular::communicate_worker()
{
    PTPLib::net::SMTS_Event smts_event;
    std::future<SolverProcess::Result>  future_solverResult;
    try {
#ifdef VERBOSE_THREAD
        communicator_thread_id = std::this_thread::get_id();
#endif
        if (future_solverResult.valid()) {
            if (log_enabled)
                synced_stream.println_bold(log_enabled ? PTPLib::common::Color::FG_Cyan : PTPLib::common::Color::FG_DEFAULT,
                                           " Future is valid");
            exit(EXIT_FAILURE);
        }
        while (true) {
            std::unique_lock<std::mutex> lk(getChannel().getMutex());
            getChannel().wait_event_solver_reset(lk);
#ifdef VERBOSE_THREAD
            if (communicator_thread_id != std::this_thread::get_id())
                    throw PTPLib::common::Exception(__FILE__, __LINE__, std::string(__FUNCTION__) +" has inconsistent thread id");
#endif

            assert([&]() {
                if (not lk.owns_lock()) {
                    throw PTPLib::common::Exception(__FILE__, __LINE__, std::string(__FUNCTION__) + " can't take the lock");
                }
                return true;
            }());

            if (getChannel().shallStop()) {
                getChannel().clearShallStop();
                lk.unlock();

                if (future_solverResult.valid()) {
                    SolverProcess::Result res = future_solverResult.get();
                    assert(res != SolverProcess::Result::UNKNOWN);
                    std::string res_str = SolverProcess::resultToString(res);
                    {
                        std::scoped_lock<std::mutex> s_lk(getChannel().getMutex());
                        auto current_header = channel.get_current_header({PTPLib::common::Param.NAME, PTPLib::common::Param.NODE});
                        if (log_enabled)
                            synced_stream.println_bold(log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                                                       "[ t ", __func__, "] -> ", " Shall Stop Report " +
                                                                                  res_str, current_header.at(PTPLib::common::Param.NODE));
                        net::Report::report(get_SMTS_server(), current_header, res_str);
                    }
                }

            } else if (not getChannel().isEmpty_query()) {
                smts_event = getChannel().pop_front_query();
                if (not smts_event.header.count(PTPLib::common::Param.NAME) or not smts_event.header.count(PTPLib::common::Param.NODE))
                    throw PTPLib::common::Exception(__FILE__, __LINE__, "missing mandatory key in header");
                if (log_enabled)
                    synced_stream.println_bold(log_enabled ? PTPLib::common::Color::FG_Cyan : PTPLib::common::Color::FG_DEFAULT,
                                               "[ t ", __func__, "] -> ", "Updating the channel with ",
                                               smts_event.header.at(PTPLib::common::Param.COMMAND), " and waiting");
                lk.unlock();
                assert(not smts_event.header.empty());
                if (request_solver_toStop(smts_event)) {
                    if (future_solverResult.valid()) {
                        future_solverResult.wait();
                        SolverProcess::Result result = future_solverResult.get();
                        std::string result_str = SolverProcess::resultToString(result);
                        if (result != SolverProcess::Result::UNKNOWN) {
                            std::scoped_lock<std::mutex> s_lk(getChannel().getMutex());
                            auto current_header = channel.get_current_header({PTPLib::common::Param.NAME, PTPLib::common::Param.NODE});
                            if (log_enabled)
                                synced_stream.println_bold(
                                        log_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                                        "[ t ", __func__, "] -> ", " Result ", result_str);
                            net::Report::report(get_SMTS_server(), current_header, result_str);
                        }
                    }
                }

                bool should_resume;
                bool shouldUpdateSolverAddress = false;
                {
                    getChannel().clearShouldStop();
                    std::scoped_lock<std::mutex> s_lk(getChannel().getMutex());
                    should_resume = execute_event(smts_event, shouldUpdateSolverAddress);
                }
                if (should_resume) {
                    assert(not smts_event.header.at(PTPLib::common::Param.QUERY).empty());
                    future_solverResult = thread_pool.submit([this, smts_event, shouldUpdateSolverAddress] {
                        return solver_process->solve(smts_event, shouldUpdateSolverAddress);
                    }, ::get_task_name(PTPLib::common::TASK::SOLVER));
                } else {
                    delete_solver_process();
                    break;
                }
            } else if (getChannel().shouldReset())
                break;

            else {
                synced_stream.println(log_enabled ? PTPLib::common::Color::FG_Cyan : PTPLib::common::Color::FG_DEFAULT,
                                      "[t COMMUNICATOR ] -> ", "spurious wake up!");
                assert(false);
            }
        }
    }
    catch (std::exception & ex)
    {
        net::Report::error(get_SMTS_server(), smts_event.header, std::string(ex.what()) + " from: "+ __func__ );
    }
}

bool Schedular::execute_event(PTPLib::net::SMTS_Event & smts_event, bool & shouldUpdateSolverAddress) {
    assert(not smts_event.header.empty());
    if (smts_event.header.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.STOP)
        return false;

    else if (smts_event.header.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.SOLVE) {
        net::Report::info(get_SMTS_server(), smts_event, "start");
        if (log_enabled)
            synced_stream.println(log_enabled ? PTPLib::common::Color::FG_Cyan : PTPLib::common::Color::FG_DEFAULT,
                              "[ t ", __func__, "] -> ", " SOLVER START TO SOLVE ON ", smts_event.header.at(PTPLib::common::Param.NAME));
        if (not (solver_process = new SolverProcess(synced_stream, SMTS_server_socket, getChannel())))
            throw PTPLib::common::Exception(__FILE__, __LINE__, ";SolverProcess: out of memory");
        solver_process->init(smts_event);
        smts_event.body.clear();
        shouldUpdateSolverAddress = true;
        getChannel().set_current_header(smts_event.header);
    }
    else if (smts_event.header.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.PARTITION) {
            solver_process->partition(smts_event,
                                      (uint8_t) atoi(smts_event.header.at(PTPLib::common::Param.PARTITIONS).c_str()));
    }
    else if (smts_event.header.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.CLAUSEINJECTION) {
            auto pulled_clauses = channel.swap_pulled_clauses();
            solver_process->injectPulledClauses(pulled_clauses, smts_event.header.at(PTPLib::common::Param.NODE));
    }
    else if (smts_event.header.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.INCREMENTAL) {
        if (smts_event.header.count(PTPLib::common::Param.NODE_) and smts_event.header.count(PTPLib::common::Param.QUERY)) {
            if (log_enabled)
                net::Report::info(get_SMTS_server(), smts_event,
                             "incremental solving step from " + smts_event.header[PTPLib::common::Param.NODE]);
            if (solver_process->forked) {
                solver_process->kill_partition();
                solver_process->forked = false;
            }
            shouldUpdateSolverAddress = true;
            smts_event.header[PTPLib::common::Param.NODE] = smts_event.header.at(PTPLib::common::Param.NODE_);
            smts_event.header.erase(PTPLib::common::Param.NODE_);
            getChannel().set_current_header(smts_event.header);
        }
    }

    else if (smts_event.header.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.CNFCLAUSES) {
        // TODO
//            solver->getCnfClauses(smts_event);
        }

    else if (smts_event.header.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.CNFLEARNTS) {
        // TODO
        }

    return true;
}

bool Schedular::request_solver_toStop(PTPLib::net::SMTS_Event const & SMTS_Event) {
    assert(not SMTS_Event.header.at(PTPLib::common::Param.COMMAND).empty());
    if (SMTS_Event.header.at(PTPLib::common::Param.COMMAND) != PTPLib::common::Command.SOLVE) {
        channel.setShouldStop();
        return true;
    } else return false;
}

void Schedular::queue_event(PTPLib::net::SMTS_Event && event) {
    getChannel().push_back_query(std::move(event));
    getChannel().notify_all();
}

void Schedular::notify_reset() {
    std::unique_lock<std::mutex> _lk(getChannel().getMutex());
    if (not _lk.owns_lock()) {
        throw PTPLib::common::Exception(__FILE__, __LINE__, std::string(__FUNCTION__) + " can't take the lock");
    };
    channel.setReset();
    channel.notify_all();
    _lk.unlock();
}



void Schedular::push_clause_worker(int seed, int min, int max) {
#ifdef VERBOSE_THREAD
    push_thread_id = std::this_thread::get_id();
#endif
    int push_duration = min + (seed % (max - min + 1));
    if (log_enabled)
        synced_stream.println_bold(log_enabled ? PTPLib::common::Color::FG_Blue : PTPLib::common::Color::FG_DEFAULT,
                               "[ t ", __func__, "] -> ", " timout : ", push_duration, " ms");
    PTPLib::net::time_duration wakeupAt = std::chrono::milliseconds(push_duration);
    while (true) {
        if (log_enabled)
            PTPLib::common::PrintStopWatch psw("[t PUSH ] -> measured wait and write duration: ", synced_stream,
                                           log_enabled ? PTPLib::common::Color::FG_Blue : PTPLib::common::Color::FG_DEFAULT);
        std::unique_lock<std::mutex> lk(getChannel().getMutex());
        bool reset = getChannel().wait_for_reset(lk, wakeupAt);
#ifdef VERBOSE_THREAD
        if (push_thread_id != std::this_thread::get_id())
                throw PTPLib::common::Exception(__FILE__, __LINE__, std::string(__FUNCTION__) +" has inconsistent thread id");
#endif

        assert([&]() {
            if (not lk.owns_lock()) {
                throw PTPLib::common::Exception(__FILE__, __LINE__, std::string(__FUNCTION__) + " can't take the lock");
            }
            return true;
        }());
        if (not reset) {
            if (not getChannel().empty_learned_clauses()) {
                auto map_branch_clauses = getChannel().swap_learned_clauses();
                getChannel().clear_learned_clauses();
                auto header = channel.get_current_header({PTPLib::common::Param.NAME, PTPLib::common::Param.NODE, PTPLib::common::Param.QUERY});
                lk.unlock();

                assert([&]() {
                    if (lk.owns_lock()) {
                        throw PTPLib::common::Exception(__FILE__, __LINE__, std::string(__FUNCTION__) + " should not hold the lock");
                    }
                    return true;
                }());
                if (not header.empty()) {
                    lemmas_publish(map_branch_clauses, header);
                    map_branch_clauses->clear();
                }
            } else {
                if (log_enabled)
                    synced_stream.println(
                            log_enabled ? PTPLib::common::Color::FG_Blue : PTPLib::common::Color::FG_DEFAULT,
                            "[ t ", __func__, "] -> ", " Channel empty!");
            }
        } else if (getChannel().shouldReset())
            break;

        else {
            synced_stream.println(log_enabled ? PTPLib::common::Color::FG_Blue : PTPLib::common::Color::FG_DEFAULT,
                                  "[ t ", __func__, "] -> ", "spurious wake up!");
            assert(EXIT_FAILURE);
        }
    }
}

void Schedular::lemmas_publish(std::unique_ptr<PTPLib::net::map_solver_clause> const & map_branch_clause, PTPLib::net::Header & header) {
    assert((not header.at(PTPLib::common::Param.NODE).empty()) and (not header.at(PTPLib::common::Param.NAME).empty()));
    for (const auto & branch_clause : *map_branch_clause) {
        header[PTPLib::common::Param.NODE] = branch_clause.first;
        header[PTPLib::common::Command.LEMMAS] = "+" + std::to_string(branch_clause.second.size());
        lemma_push(branch_clause.second, header);
        if (log_enabled)
            synced_stream.println_bold(log_enabled ? PTPLib::common::Color::FG_Blue : PTPLib::common::Color::FG_DEFAULT,
                                       "[ t ", __func__, "] -> ", " push learned clauses to Cloud Clause Size: ", branch_clause.second.size());
    }
}