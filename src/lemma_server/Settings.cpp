//
// Created by Matteo Marescotti on 02/12/15.
//

#include <getopt.h>
#include <iostream>
#include "Settings.h"


Settings::Settings() :
        port(5000),
        server(std::string(), 0) {}

void Settings::load(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "hp:s:d:")) != -1)
        switch (opt) {
            case 'h':
                std::cout << "Usage: " << argv[0] << "\n";
                exit(0);
            case 'p':
                this->port = (uint16_t) atoi(optarg);
                break;
            case 's':
                this->server = optarg;
                break;
            case 'd':
                this->db_filename = std::string(optarg);
                break;
            default:
                std::cout << "unknown option '" << opt << "'" << "\n";
                exit(-1);
        }

    for (int i = optind; i < argc; i++) {
    }
}
