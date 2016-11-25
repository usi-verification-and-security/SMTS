//
// Created by Matteo on 21/07/16.
//

#ifndef CLAUSE_SHARING_SETTINGS_H
#define CLAUSE_SHARING_SETTINGS_H

#include <list>
#include <map>


class Settings {
private:
    void load_header(std::map<std::string, std::string> &header, char *string);

public:
    Settings();

    void load(int, char **);

    std::string server;
    std::string lemmas;
    std::list<std::string> files;
    bool clear_lemmas;
    std::map<std::string, std::string> header_solve;
};

#endif //CLAUSE_SHARING_SETTINGS_H
