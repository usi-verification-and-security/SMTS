//
// Author: Matteo Marescotti
//

#include <getopt.h>
#include <iostream>
#include "Settings.h"


Settings::Settings() :
        send_again(false),
        port(5000) {}

void Settings::load(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "hap:s:d:")) != -1)
        switch (opt) {
            case 'h':
                std::cout << "Usage: " << argv[0] << "\n";
                exit(0);
            case 'a':
                this->send_again = true;
                break;
            case 'p':
                this->port = (uint16_t) atoi(optarg);
                break;
            case 's':
                this->server = optarg;
                break;
            case 'd':
                this->db_filename = optarg;
                break;
            default:
                std::cout << "unknown option '" << opt << "'" << "\n";
                exit(-1);
        }

    for (int i = optind; i < argc; i++) {
    }
}
