//
// Author: Matteo Marescotti
//

#ifndef SMTS_CLIENT_SETTINGS_H
#define SMTS_CLIENT_SETTINGS_H

#include <list>
#include <map>
#include "PTPLib/net/Header.hpp"


class Settings {
public:
    Settings();

    void load(int, char **);
    bool verbose;
    std::string server;
    std::string lemmas;
    std::list<std::string> files;
    bool keep_lemmas;
    bool dump_clauses;
    PTPLib::net::Header parameters;
};

#endif
