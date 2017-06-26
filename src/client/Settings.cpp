//
// Author: Matteo Marescotti
//

#include <getopt.h>
#include <sstream>
#include "lib/lib.h"
#include "Settings.h"


Settings::Settings() :
        verbose(false),
        keep_lemmas(false) {}

void Settings::load(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "hvs:l:kp:r:")) != -1)
        switch (opt) {
            case 'h':
                new(this) Settings();
                std::cout << "Usage: " << argv[0] << "\n"
                        "[-h] display this message\n"
                        "[-s server-host:port] if empty then file mode is enabled\n"
                        "[-l lemma_server-host:port]\n"
                        "[-v] verbose\n"
                        "[-k] (only for file mode) keep lemmas in lemma server after solving\n"
                        "[-p parameter-json] (only for file mode)\n"
                        "[-r parameter-key=value] (only for file mode)\n"
                        "[file1 ...] (only for file mode)\n";
                exit(0);

            case 's':
                this->server = optarg;
                break;
            case 'l':
                this->lemmas = optarg;
                break;
            case 'v':
                this->verbose = true;
                break;
            case 'k':
                this->keep_lemmas = true;
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