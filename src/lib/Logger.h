//
// Author: Matteo Marescotti
//

#ifndef SMTS_LIB_LOGGER_H
#define SMTS_LIB_LOGGER_H

#include <assert.h>
#include <ctime>
#include <iostream>
#include <sstream>
#include <mutex>
#include <iomanip>


typedef uint8_t log_level;

class Logger {
private:
    Logger() {}

public:
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
};


#endif
