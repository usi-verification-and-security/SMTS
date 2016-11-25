//
// Created by Matteo on 23/08/16.
//

#ifndef CLAUSE_SERVER_SQLITE3_H
#define CLAUSE_SERVER_SQLITE3_H

#include <functional>
#include <string>
#include <vector>
#include <map>
#include "Exception.h"


namespace SQLite3 {
    class Exception : public ::Exception {
    public:
        explicit Exception(const char *message) : Exception(std::string(message)) {}

        explicit Exception(const std::string &message) : ::Exception("SQLiteException: " + message) {}
    };


    class Statement {
    private:
        void *stmt;
    public:
        Statement(void *);

        ~Statement();

        void bind(int);

        void bind(int, int);

        void bind(int, const char *, int);

        void bind(int, char *, int, void(*)(void *));

        void reset();

        void clear();

        void exec();

    };


    class Connection {
    private:
        void *db;
    public:
        Connection(const std::string &db_filename);

        ~Connection();

        Statement *prepare(const std::string &, int _ = -1);

        void exec(const std::string &, std::function<int(int, char **, char **)>);

        void exec(const std::string &);
    };
}

#endif //CLAUSE_SERVER_SQLITE3_H
