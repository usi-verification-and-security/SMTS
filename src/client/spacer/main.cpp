//
// Created by Matteo on 24/10/2016.
//

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <lib/net.h>
#include "fixedpoint.h"
#include "lib/lib.h"


int main(){
    SQLite3::Connection conn;
    conn.exec("CREATE TABLE IF NOT EXISTS Push"
                      "(id INTEGER PRIMARY KEY, "
                      "ts INTEGER, "
                      "data TEXT);");
    auto stmt = *conn.prepare(
            "INSERT INTO Push (ts,data) VALUES(?,?);");
    stmt.bind(1, std::time(nullptr));
    stmt.bind(2, "ciao");
    stmt.exec();
    stmt = *conn.prepare(
            "INSERT INTO Push (ts,data) VALUES(?,?);");
    stmt.bind(1, std::time(nullptr));
    stmt.bind(2, "ciao");
    stmt.exec();
    std::cout << conn.last_rowid();
}