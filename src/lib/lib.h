//
// Created by Matteo on 11/08/16.
//

#ifndef CLAUSE_SERVER_LIB_H
#define CLAUSE_SERVER_LIB_H

#include <string>
#include <sstream>
#include <vector>
#include <functional>


void split(const std::string &, const std::string &, std::vector<std::string> &, uint32_t limit = 0);

void split(const std::string &, const std::string &, std::function<void(const std::string &)>, uint32_t limit = 0);

template<typename T>
void join(std::stringstream &stream, const std::string &delimiter, const std::vector<T> &vector) {
    for (auto it = vector.begin(); it != vector.end(); ++it) {
        stream << *it;
        if (it + 1 != vector.end())
            stream << delimiter;
    }
}

void replace(std::string &, const std::string &, const std::string &);


#include "Exception.h"
#include "Log.h"
#include "Process.h"
#include "Thread.h"
#include "sqlite3.h"
#include "net.h"

#endif //CLAUSE_SERVER_LIB_H
