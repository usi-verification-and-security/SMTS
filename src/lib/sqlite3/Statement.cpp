//
// Created by Matteo on 02/12/2016.
//

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

    void Statement::bind(int pos, int value) {
        if (sqlite3_bind_int((sqlite3_stmt *) this->stmt, pos, value) != SQLITE_OK)
            throw Exception("bind int failed");
    }

    void Statement::bind(int pos, const char *value, int len) {
        if (sqlite3_bind_text((sqlite3_stmt *) this->stmt, pos, value, len, SQLITE_STATIC) != SQLITE_OK)
            throw Exception("bind const text failed");
    }

    void Statement::bind(int pos, char *value, int len, void(*destructor)(void *)) {
        if (sqlite3_bind_text((sqlite3_stmt *) this->stmt, pos, value, len, destructor) != SQLITE_OK)
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
    }
}