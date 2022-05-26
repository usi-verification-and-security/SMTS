/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */


#include "Settings.h"

#include <getopt.h>
#include <iostream>
#include <string>

Settings::Settings()
: send_again(false)
, port(5000) {}

void Settings::load(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "hap:s:d:")) != -1)
        switch (opt) {
            case 'h':
                new(this) Settings();
                std::cout << "\n******* Compiled with " << __VERSION__ << " on " << __DATE__ << " ******\n"
                          << "SMTS version: " << SMTS_VERSION << "\n"
                        "Usage: " << argv[0] << "\n"
                                  "[-h] display this message\n"
                                  "[-s server-host:port]\n"
                                  "[-p listening-port] default: " << this->port << "\n"
                                  "[-d lemma_database-file] not used if not set\n"
                                  "[-a] send lemmas again to solvers\n"
                                  "\n";
                exit(0);
            case 's':
                this->server = optarg;
                break;
            case 'p':
                this->port = (uint16_t) atoi(optarg);
                break;
            case 'd':
                this->db_filename = optarg;
                break;
            case 'a':
                this->send_again = true;
                break;
            default:
                std::cout << "unknown option '" << opt << "'" << "\n";
                exit(-1);
        }
}
