//
// Created by Matteo on 23/08/16.
//

#include <sqlite3.h>
#include "lib/lib.h"
#include "sqlite3.h"


SQLite3Stmt::SQLite3Stmt(void *stmt) : stmt(stmt) { }

SQLite3Stmt::~SQLite3Stmt() {
    sqlite3_finalize((sqlite3_stmt *) this->stmt);
}

void SQLite3Stmt::bind(int pos) {
    if (sqlite3_bind_null((sqlite3_stmt *) this->stmt, pos) != SQLITE_OK)
        throw SQLiteException("bind null failed");
}

void SQLite3Stmt::bind(int pos, int value) {
    if (sqlite3_bind_int((sqlite3_stmt *) this->stmt, pos, value) != SQLITE_OK)
        throw SQLiteException("bind int failed");
}

void SQLite3Stmt::bind(int pos, const char *value, int len) {
    if (sqlite3_bind_text((sqlite3_stmt *) this->stmt, pos, value, len, SQLITE_STATIC) != SQLITE_OK)
        throw SQLiteException("bind const text failed");
}

void SQLite3Stmt::bind(int pos, char *value, int len, void(*destructor)(void *)) {
    if (sqlite3_bind_text((sqlite3_stmt *) this->stmt, pos, value, len, destructor) != SQLITE_OK)
        throw SQLiteException("bind const text failed");
}

void SQLite3Stmt::reset() {
    if (sqlite3_reset((sqlite3_stmt *) this->stmt) != SQLITE_OK) {
        throw SQLiteException("could not reset the statement.");
    }
}

void SQLite3Stmt::clear() {
    if (sqlite3_clear_bindings((sqlite3_stmt *) this->stmt) != SQLITE_OK) {
        throw SQLiteException("could not clear the statement.");
    }
}

void SQLite3Stmt::exec() {
    if (sqlite3_step((sqlite3_stmt *) this->stmt) != SQLITE_DONE) {
        throw SQLiteException("could not execute the statement.");
    }
}


SQLite3::SQLite3(std::string &db_filename) {
    int rc = sqlite3_open(db_filename.c_str(), (sqlite3 **) &this->db);
    if (rc)
        throw SQLiteException(std::string("can't open: ") + db_filename);
}

SQLite3::~SQLite3() {
    sqlite3_close((sqlite3 *) this->db);
}

SQLite3Stmt *SQLite3::prepare(const std::string &sql, int n) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare(
            (sqlite3 *) this->db,
            sql.c_str(),
            n,
            &stmt,
            0) != SQLITE_OK) {
        throw SQLiteException("could not prepare the statement.");
    }
    return new SQLite3Stmt(stmt);
}

void SQLite3::exec(const std::string &sql, std::function<int(int, char **, char **)> callback_row) {
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
        throw SQLiteException(err);
    }
}

void SQLite3::exec(const std::string &sql) {
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
        throw SQLiteException(err);
    }
}
