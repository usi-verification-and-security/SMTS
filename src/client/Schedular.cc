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
