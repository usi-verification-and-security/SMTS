//
// Created by Matteo Marescotti on 02/12/15.
//

#include <getopt.h>
#include <sstream>
#include "lib/Logger.h"
#include "Settings.h"


Settings::Settings() :
        verbose(false),
        clear_lemmas(false) {}

void Settings::load(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "hvs:l:cp:r:")) != -1)
        switch (opt) {
            case 'h':
                std::cout << "Usage: " << argv[0] <<
                          " [-s server-host:port]"
                                  "[-l lemma_server-host:port]"
                                  "[-hvc]"
                                  "[-p parameter-json]"
                                  "[-r parameter-key=value]"
                                  "\n";
                exit(0);
            case 'v':
                this->verbose = true;
                break;
            case 's':
                this->server = optarg;
                break;
            case 'l':
                this->lemmas = optarg;
                break;
            case 'c':
                this->clear_lemmas = true;
                break;
            case 'p':
                std::istringstream(optarg) >> this->parameters;
                break;
            case 'r':
                int i;
                for (i = 0; optarg[i] != '=' && optarg[i] != '\0' && i < (uint8_t) -1; i++) {
                }
                if (optarg[i] != '=') {
                    Logger::log(Logger::ERROR, std::string("bad pair: ") + optarg);
                }
                optarg[i] = '\0';
                this->parameters[optarg] = &optarg[i + 1];
                break;
            default:
                std::cout << "unknown option '" << opt << "'" << "\n";
                exit(-1);
        }
    for (int i = optind; i < argc; i++) {
        this->files.push_back(argv[i]);
    }
}