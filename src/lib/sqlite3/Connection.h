//
// Created by Matteo on 02/12/2016.
//

#ifndef CLAUSE_SERVER_CONNECTION_H
#define CLAUSE_SERVER_CONNECTION_H

#include "Statement.h"


namespace SQLite3 {
    class Connection {
    private:
        void *db;
    public:
        Connection() : Connection("") {}

        Connection(const std::string &);

        ~Connection();

        Statement *prepare(const std::string &, int _ = -1);

        void exec(const std::string &, std::function<int(int, char **, char **)>);

        void exec(const std::string &);

        int64_t last_rowid();
    };
}

#endif //CLAUSE_SERVER_CONNECTION_H
