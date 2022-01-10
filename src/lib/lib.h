//
// Author: Matteo Marescotti
//

#ifndef SMTS_LIB_LIB_H
#define SMTS_LIB_LIB_H

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <functional>


std::istream &split(std::istream &stream,
                    const char delimiter,
                    std::function<void(const std::string &)> callback);

std::istream &split(std::istream &stream,
                    const char delimiter,
                    std::vector<std::string> &vector);

std::vector<std::string> split(const std::string &, const std::string &, uint32_t limit = 0);

void split(const std::string &, const std::string &, std::function<void(const std::string &)>, uint32_t limit = 0);

template<typename T>
std::ostream &join(std::ostream &stream, const std::string &delimiter, const std::vector<T> &vector) {
    for (auto it = vector.begin(); it != vector.end(); ++it) {
        stream << *it;
        if (it + 1 != vector.end())
            stream << delimiter;
    }
    return stream;
}

std::string &replace(std::string &, const std::string &, const std::string &, size_t n = 0);

std::string &operator%(std::string &, const std::pair<std::string, std::string> &);

template<typename T>
std::ostream &operator<<(std::ostream &stream, const std::vector<T> &v) {
    for (auto &i:v) {
        stream << i << '\0';
    }
    return stream;
}

template<typename T>
std::istream &operator>>(std::istream &stream, std::vector<T> &v) {
    return ::split(stream, '\0', [&](const std::string &sub) {
        if (sub.size() == 0)
            return;
        T t;
        std::istringstream(sub) >> t;
        v.push_back(t);
    });
}

template<typename T>
const std::string to_string(const T &obj) {
    std::ostringstream ss;
    ss << obj;
    return ss.str();
}
namespace PartitionChannel {

    struct pair {
        std::string clause;
        int level;
    };

    static struct {
        const std::string Partition = "partition";
        const std::string Stop = "stop";
        const std::string ClauseInjection = "inject";
        const std::string Incremental = "incremental";
        const std::string CnfClauses = "cnf-clauses";
        const std::string Cnflearnts = "cnf-learnts";
        const std::string Solve = "solve";
        const std::string Lemmas = "lemmas";
    } Command;

    struct Task {
        const enum {
            incremental, resume, partition
        } command;
        std::string smtlib;
    };

    enum ThreadName {
        Comunication, ClausePull, ClausePush, MemCheck
    };
    enum Status {
        unknown, sat, unsat
    };
//    namespace SMTS {
//        extern bool colorMode;
//    }
}
#include "Exception.h"
#include "Logger.h"
#include "Process.h"
#include "SMTSThread.h"
#include "sqlite3.h"
#include "net.h"
#include "memory.h"

#endif
