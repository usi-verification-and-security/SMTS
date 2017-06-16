//
// Author: Matteo Marescotti
//

#ifndef SMTS_CLIENT_SETTINGS_H
#define SMTS_CLIENT_SETTINGS_H

#include <list>
#include <map>
#include "lib/net/Header.h"


class Settings {
public:
    Settings();

    void load(int, char **);

    bool verbose;
    std::string server;
    std::string lemmas;
    std::list<std::string> files;
    bool clear_lemmas;
    net::Header parameters;
};

#endif
