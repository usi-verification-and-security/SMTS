/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */


#ifndef SMTS_LIB_LOGGER_H
#define SMTS_LIB_LOGGER_H

#include <cassert>
#include <ctime>
#include <iostream>
#include <sstream>
#include <mutex>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace std;

typedef uint8_t log_level;

class Logger {
private:
    Logger() { }
public:
    static void writeIntoFile(bool parent,string type,string description,pid_t processId) {
        static std::mutex mtx;
        mtx.lock();
        string fileName;
        if(parent)
            fileName="logs/Parent-"+to_string(processId)+".txt";
        else
            fileName="logs/Child-"+to_string(processId)+".txt";
        string filename(fileName);
        fstream file;
        std::time_t time = std::time(nullptr);
        struct tm tm = *std::localtime(&time);

        file.open(filename, std::ios_base::app | std::ios_base::in);
        if (file.is_open()) {

            file <<""<< type << endl;
            file <<" "<< description << endl;
            file <<" time: " <<std::put_time(&tm, "%H:%M:%S") << "\t";
            file <<endl<<endl<<endl;
        }
        mtx.unlock();
    }

    static void build_SolverInputPath(bool exist, bool parent, const std::string & smt_lib, std::string solver_address, pid_t processId) {
        static std::mutex mtx;
        mtx.lock();
        string fileName;
        if (parent)
            fileName = "logs/Parent-" + processId + solver_address + ".smt2";
        else
            fileName = "logs/Child-" + processId + solver_address  + ".smt2";

        string filename(fileName);
        ofstream file;
        if (exist)
            file.open(fileName, ios::out | ios::trunc);
        file.open(filename, ios::app | ios::out);
        if (file.is_open()) {

            file << smt_lib << endl;
            file.close();
        }
        mtx.unlock();
    }

    template<typename T>
    static void log(log_level level, const T &message) {
        static std::mutex mtx;
        std::lock_guard<std::mutex> _l(mtx);
        int r = 0;
        std::ostringstream stream;
        std::time_t time = std::time(nullptr);
        struct tm tm = *std::localtime(&time);
        stream << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\t";
        switch (level) {
            case INFO:
                stream << "INFO\t";
                r += system("tput setaf 3");
                r += system("tput bold");
                break;
            case WARNING:
                if (getenv("TERM")) {
                    r += system("tput setaf 3");
                    r += system("tput bold");
                }
                stream << "WARNING\t";
                break;
            case ERROR:
                if (getenv("TERM")) {
                    r += system("tput setaf 9");
                    r += system("tput bold");
                }
                stream << "ERROR\t";
                break;
            case PUSH:
                if (getenv("TERM")) {
                    r += system("tput setaf 2");
                    r += system("tput bold");
                }
                stream << "PUSH\t";
                break;
            case PULL:
                if (getenv("TERM")) {
                    r += system("tput setaf 9");
                    r += system("tput bold");
                }
                stream << "PULL\t";
                break;
            case ASSERT:
                if (getenv("TERM")) {
                    r += system("tput setaf 3");
                    r += system("tput bold");
                }
                stream << "ASSERT\t";
                break;
            default:
                stream << "UNKNOWN\t";
        }
        stream << message;
        std::cout << stream.str() << std::endl;
        if (getenv("TERM")) {
            r = system("tput sgr0");
        }
    }

    static const log_level INFO = 1;
    static const log_level WARNING = 2;
    static const log_level ERROR = 3;
    static const log_level PUSH = 4;
    static const log_level PULL = 5;
    static const log_level ASSERT = 6;
};


#endif
