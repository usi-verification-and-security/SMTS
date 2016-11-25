//
// Created by Matteo on 21/07/16.
//

#ifndef CLAUSE_SHARING_CLAUSESERVER_H
#define CLAUSE_SHARING_CLAUSESERVER_H

#include <map>
#include <ctime>
#include "lib/net.h"
#include "lib/sqlite3.h"
#include "Lemma.h"
#include "Settings.h"
#include "Node.h"


class LemmaServer : public net::Server {
private:
    std::shared_ptr<net::Socket> server;
    std::shared_ptr<SQLite3::Connection> db;
    std::map<std::string, Node> lemmas;                            // name -> lemmas
    std::map<std::string, std::map<std::string, std::list<Lemma *>>> solvers;  // name -> solver -> lemmas
protected:
    void handle_accept(net::Socket &);

    void handle_close(net::Socket &);

    void handle_message(net::Socket &, std::map<std::string, std::string> &, std::string &);

    void handle_exception(net::Socket &, net::SocketException &);

public:
    LemmaServer(uint16_t, const std::string &, const std::string &);
};

#endif //CLAUSE_SHARING_CLAUSESERVER_H
