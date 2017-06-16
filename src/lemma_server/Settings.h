//
// Author: Matteo Marescotti
//

#ifndef SMTS_LEMMASERVER_SETTINGS_H
#define SMTS_LEMMASERVER_SETTINGS_H

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

#endif
