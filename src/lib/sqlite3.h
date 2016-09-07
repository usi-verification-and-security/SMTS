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


class SQLiteException : public Exception {
public:
    explicit SQLiteException(const char *message) : SQLiteException(std::string(message)) { }

    explicit SQLiteException(const std::string &message) : Exception("SQLiteException: " + message) { }
};


class SQLite3Stmt {
private:
    void *stmt;
public:
    SQLite3Stmt(void *);

    ~SQLite3Stmt();

    void bind(int);

    void bind(int, int);

    void bind(int, const char *, int);

    void bind(int, char *, int, void(*)(void *));

    void reset();

    void clear();

    void exec();

};


class SQLite3 {
private:
    void *db;
public:
    SQLite3(std::string &db_filename);

    ~SQLite3();

    SQLite3Stmt *prepare(const std::string &, int _ = -1);

    void exec(const std::string &, std::function<int(int, char **, char **)>);

    void exec(const std::string &);
};

#endif //CLAUSE_SERVER_SQLITE3_H
