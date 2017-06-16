//
// Author: Matteo Marescotti
//

#ifndef SMTS_LIB_SQLITE3_CONNECTION_H
#define SMTS_LIB_SQLITE3_CONNECTION_H

#include <functional>
#include "Statement.h"


namespace SQLite3 {
    class Connection {
    private:
        const void *db;
    public:
        Connection() : Connection("") {}

        Connection(const std::string &);

        ~Connection();

        Statement *prepare(const std::string &, int _ = -1) const;

        void exec(const std::string &, std::function<int(int, char **, char **)>) const;

        void exec(const std::string &) const;

        int64_t last_rowid() const;
    };
}

#endif
