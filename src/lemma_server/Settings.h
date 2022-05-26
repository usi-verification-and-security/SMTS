/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMTS_LEMMASERVER_SETTINGS_H
#define SMTS_LEMMASERVER_SETTINGS_H

#include <string>


class Settings {
public:
    Settings();

    void load(int, char **);

    bool send_again;
    uint16_t port;
    std::string server;
    std::string db_filename;
};

#endif
