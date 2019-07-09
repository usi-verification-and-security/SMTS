//
// Author: Matteo Marescotti
//

#include <sqlite3.h>
#include "Exception.h"
#include "Connection.h"


namespace SQLite3 {
    Connection::Connection(const std::string &db_filename) {
        const char *filename = db_filename.size() ? db_filename.c_str() : ":memory:";
        int rc = sqlite3_open(filename, (sqlite3 **) &this->db);
        if (rc)
            throw Exception(__FILE__, __LINE__, std::string("can't open: ") + filename);
    }

    Connection::~Connection() {
        sqlite3_close((sqlite3 *) this->db);
    }

    Statement *Connection::prepare(const std::string &sql, int n) const {
        sqlite3_stmt *stmt;
        if (sqlite3_prepare(
                (sqlite3 *) this->db,
                sql.c_str(),
                n,
                &stmt,
                0) != SQLITE_OK) {
            throw Exception(__FILE__, __LINE__, "could not prepare the statement.");
        }
        return new Statement(stmt);
    }

    void Connection::exec(const std::string &sql, std::function<int(int, char **, char **)> callback_row) const {
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
            throw Exception(__FILE__, __LINE__, err);
        }
    }

    void Connection::exec(const std::string &sql) const {
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
            throw Exception(__FILE__, __LINE__, err);
        }
    }

    int64_t Connection::last_rowid() const {
        return sqlite3_last_insert_rowid((sqlite3 *) this->db);
    }
}