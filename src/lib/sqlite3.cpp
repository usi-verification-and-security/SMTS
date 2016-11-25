//
// Created by Matteo on 23/08/16.
//

#include <sqlite3.h>
#include "lib/lib.h"
#include "sqlite3.h"


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


    Connection::Connection(const std::string &db_filename) {
        int rc = sqlite3_open(db_filename.c_str(), (sqlite3 **) &this->db);
        if (rc)
            throw Exception(std::string("can't open: ") + db_filename);
    }

    Connection::~Connection() {
        sqlite3_close((sqlite3 *) this->db);
    }

    Statement *Connection::prepare(const std::string &sql, int n) {
        sqlite3_stmt *stmt;
        if (sqlite3_prepare(
                (sqlite3 *) this->db,
                sql.c_str(),
                n,
                &stmt,
                0) != SQLITE_OK) {
            throw Exception("could not prepare the statement.");
        }
        return new Statement(stmt);
    }

    void Connection::exec(const std::string &sql, std::function<int(int, char **, char **)> callback_row) {
        char *errc;
        int rc = sqlite3_exec(
                (sqlite3 *) this->db,
                sql.c_str(),
                [](void *callback, int n, char **row, char **header) -> int {
                    return (*(std::function<int(int, char **, char **)> *) callback)(n, row, header);
                },
                &callback_row,
                &errc);
        if (rc != SQLITE_OK) {
            std::string err(errc);
            sqlite3_free(errc);
            throw Exception(err);
        }
    }

    void Connection::exec(const std::string &sql) {
        char *errc;
        int rc = sqlite3_exec(
                (sqlite3 *) this->db,
                sql.c_str(),
                nullptr,
                nullptr,
                &errc);
        if (rc != SQLITE_OK) {
            std::string err(errc);
            sqlite3_free(errc);
            throw Exception(err);
        }
    }
}