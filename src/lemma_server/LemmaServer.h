//
// Author: Matteo Marescotti
//

#ifndef SMTS_LEMMASERVER_LEMMASERVER_H
#define SMTS_LEMMASERVER_LEMMASERVER_H

#include <map>
#include <ctime>
#include "lib/net.h"
#include "lib/sqlite3.h"
#include "Lemma.h"
#include "Settings.h"
#include "Node.h"


class LemmaServer : public net::Server {
private:
    bool send_again;
    std::shared_ptr<net::Socket> server;
    std::shared_ptr<SQLite3::Connection> db;
    std::map<std::string, Node> lemmas;                            // name -> lemmas
    std::map<std::string, std::map<net::Socket *, std::map<Lemma *, bool>>> solvers;  // name -> solver -> lemma -> t/f
protected:
    void handle_accept(net::Socket &);

    void handle_close(net::Socket &);

    void handle_message(net::Socket &, net::Header &, std::string &);

    void handle_exception(net::Socket &, const net::SocketException &);

public:
    LemmaServer(uint16_t, const std::string &, const std::string &, bool send_again);
};

#endif
