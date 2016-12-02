//
// Created by Matteo on 02/12/2016.
//

#ifndef CLAUSE_SERVER_EXCEPTION_H
#define CLAUSE_SERVER_EXCEPTION_H

#include "lib/Exception.h"


namespace SQLite3 {
    class Exception : public ::Exception {
    public:
        explicit Exception(const char *message) : Exception(std::string(message)) {}

        explicit Exception(const std::string &message) : ::Exception("SQLiteException: " + message) {}
    };
}

#endif //CLAUSE_SERVER_EXCEPTION_H
