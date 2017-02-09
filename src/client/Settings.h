//
// Created by Matteo on 21/07/16.
//

#ifndef CLAUSE_SHARING_SETTINGS_H
#define CLAUSE_SHARING_SETTINGS_H

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

#endif //CLAUSE_SHARING_SETTINGS_H
