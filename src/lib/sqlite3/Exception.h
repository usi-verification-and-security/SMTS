//
// Author: Matteo Marescotti
//

#ifndef SMTS_LIB_SQLITE3_EXCEPTION_H
#define SMTS_LIB_SQLITE3_EXCEPTION_H

#include "lib/Exception.h"


namespace SQLite3 {
    class Exception : public ::Exception {
    public:
        explicit Exception(const char *file, unsigned line, const std::string &message) :
                ::Exception(file, line, "SQLiteException: " + message) {}
    };
}

#endif
