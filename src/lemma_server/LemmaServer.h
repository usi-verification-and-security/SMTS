//
// Author: Matteo Marescotti
//

#ifndef SMTS_LEMMASERVER_LEMMASERVER_H
#define SMTS_LEMMASERVER_LEMMASERVER_H

#include <map>
#include <ctime>
#include "lib/net.h"
#ifdef SQLITE_IS_ON
#include "lib/sqlite3.h"
#endif
#include "Lemma.h"
#include "Settings.h"
#include "Node.h"
#include "Server.h"
#include "lib/Thread_pool.hpp"

class LemmaServer : public Server {
private:
    bool send_again;
    std::shared_ptr<net::Socket> server;
#ifdef SQLITE_IS_ON
    std::shared_ptr<SQLite3::Connection> db;
#endif
    std::map<std::string, Node> lemmas;                            // name -> lemmas
    std::map<std::string, std::map<net::Socket *, std::map<Lemma *, bool>>> solvers;  // name -> solver -> lemma -> t/f
    std::map<std::string, bool> Processed;

protected:
    void handle_accept(net::Socket &);

    void handle_close(net::Socket &);

    void handle_message(net::Socket &, net::Header &, std::string &);

    void handle_exception(net::Socket &, const std::exception &);

public:
    LemmaServer(uint16_t, const std::string &, const std::string &, bool send_again);
};

#endif
