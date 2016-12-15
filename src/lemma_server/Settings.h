//
// Created by Matteo on 21/07/16.
//

#ifndef CLAUSE_SHARING_SETTINGS_H
#define CLAUSE_SHARING_SETTINGS_H

#include <string>


class Settings {
public:
    Settings();

    void load(int, char **);

    bool send_again;
    uint16_t port;
    std::string server;
    std::string db_filename;
};

#endif //CLAUSE_SHARING_SETTINGS_H
