/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMTS_CLIENT_FILETHREAD_H
#define SMTS_CLIENT_FILETHREAD_H

#include "lib/Thread.h"
#include "Settings.h"


class FileThread : public Thread {
private:
    Settings &settings;
    net::Socket server;

protected:
    void main();

public:
    FileThread(Settings &);

};

#endif
