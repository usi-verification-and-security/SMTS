/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMTS_LIB_REPORT_H
#define SMTS_LIB_REPORT_H

#include <iostream>
#include <string>
#include <unistd.h>

namespace net::Report {

    inline void report(net::Socket const & socket, PTPLib::net::SMTS_Event & SMTSEvent, std::string const & report) {
        if (report.size())
            SMTSEvent.header[PTPLib::common::Param.REPORT] = report;
        socket.write(SMTSEvent);
    }

    inline void report(net::Socket const & socket, PTPLib::net::Header & header, std::string const & report) {
        if (report.size())
            header[PTPLib::common::Param.REPORT] = report;
        socket.write(PTPLib::net::SMTS_Event(header));
    }

    inline void report(net::Socket const & socket, PTPLib::net::Header & header, std::string const & report, std::string & payload) {
       if (report.size())
           header[PTPLib::common::Param.REPORT] = report;
        socket.write(PTPLib::net::SMTS_Event(header, std::move(payload)));
    }

    inline void error(net::Socket const & socket, PTPLib::net::Header & header, std::string const & error) {
        report(socket, header, "error:" + error);
    }

    inline void report(net::Socket const & socket, std::vector<std::string> const & partitions, std::string const & search_counter, std::string const & status,
                PTPLib::net::SMTS_Event & SMTS_Event, char const * error_str = nullptr) {
        SMTS_Event.header[PTPLib::common::Param.SEARCH_COUNTER] = search_counter;
        SMTS_Event.header[PTPLib::common::Param.STATUS_INFO] = status;

        SMTS_Event.body = ::to_string(partitions);
        if (error_str != nullptr)
            return error(socket, SMTS_Event.header, error_str);

        report(socket, SMTS_Event, PTPLib::common::Param.PARTITIONS);
    }

    inline void info(net::Socket const & socket, PTPLib::net::SMTS_Event & SMTS_Event, std::string const & info) {
        report(socket, SMTS_Event, "info:" + info);
    }

    inline void warning(net::Socket const & socket,  PTPLib::net::Header & header, std::string const & warning) {
        report(socket, header, "warning:" + warning);
    }
};


#endif //SMTS_LIB_REPORT_H
