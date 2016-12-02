//
// Created by Matteo Marescotti on 02/12/15.
//

#include <getopt.h>
#include <iostream>
#include "lib/Logger.h"
#include "Settings.h"


Settings::Settings() :
        clear_lemmas(false) {}

void Settings::load_header(net::Header &header, char *string) {
    int i;
    for (i = 0; optarg[i] != '=' && optarg[i] != '\0' && i < (uint8_t) -1; i++) {
    }
    if (optarg[i] != '=') {
        Logger::log(Logger::ERROR, std::string("bad pair: ") + string);
    }
    optarg[i] = '\0';
    header[std::string(optarg)] = std::string(&optarg[i + 1]);
}


void Settings::load(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "hs:l:cr:")) != -1)
        switch (opt) {
            case 'h':
                std::cout << "Usage: " << argv[0] <<
                          " [-s server-host:port]"
                                  "[-l lemma_server-host:port]"
                                  "[-c]"
                                  "[-r run-header-key=value [...]]"
                                  "\n";
                exit(0);
            case 's':
                this->server = optarg;
                break;
            case 'l':
                this->lemmas = optarg;
                break;
            case 'c':
                this->clear_lemmas = true;
                break;
            case 'r':
                this->load_header(this->header_solve, optarg);
                break;
            default:
                std::cout << "unknown option '" << opt << "'" << "\n";
                exit(-1);
        }
    for (int i = optind; i < argc; i++) {
        this->files.push_back(std::string(argv[i]));
    }
}