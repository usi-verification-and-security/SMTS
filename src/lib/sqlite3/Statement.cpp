//
// Created by Matteo on 02/12/2016.
//

#include <string.h>
#include <sqlite3.h>
#include "Exception.h"
#include "Statement.h"


namespace SQLite3 {
    Statement::Statement(void *stmt) : stmt(stmt) {}

    Statement::~Statement() {
        sqlite3_finalize((sqlite3_stmt *) this->stmt);
    }

    void Statement::bind(int pos) {
        if (sqlite3_bind_null((sqlite3_stmt *) this->stmt, pos) != SQLITE_OK)
            throw Exception("bind null failed");
    }

    void Statement::bind(int pos, int64_t value) {
        if (sqlite3_bind_int64((sqlite3_stmt *) this->stmt, pos, value) != SQLITE_OK)
            throw Exception("bind int failed");
    }

    void Statement::bind(int pos, const std::string &value) {
        this->bind(pos, value.c_str(), (int) value.size());
    }

    void Statement::bind(int pos, const char *value, int len) {
        if (len < 0)
            len = (int) strlen(value);
        if (sqlite3_bind_text((sqlite3_stmt *) this->stmt, pos, strndup(value, (size_t) len), len, free) != SQLITE_OK)
            throw Exception("bind const text failed");
    }

    void Statement::reset() {
        if (sqlite3_reset((sqlite3_stmt *) this->stmt) != SQLITE_OK) {
            throw Exception("could not reset the statement.");
        }
    }

    void Statement::clear() {
        if (sqlite3_clear_bindings((sqlite3_stmt *) this->stmt) != SQLITE_OK) {
            throw Exception("could not clear the statement.");
        }
    }

    void Statement::exec() {
        if (sqlite3_step((sqlite3_stmt *) this->stmt) != SQLITE_DONE) {
            throw Exception("could not execute the statement.");
        }
        this->reset();
    }
}