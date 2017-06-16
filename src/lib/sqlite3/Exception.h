//
// Author: Matteo Marescotti
//

#ifndef SMTS_LIB_SQLITE3_EXCEPTION_H
#define SMTS_LIB_SQLITE3_EXCEPTION_H

#include "lib/Exception.h"


namespace SQLite3 {
    class Exception : public ::Exception {
    public:
        explicit Exception(const char *message) : Exception(std::string(message)) {}

        explicit Exception(const std::string &message) : ::Exception("SQLiteException: " + message) {}
    };
}

#endif
