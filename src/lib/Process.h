//
// Author: Matteo Marescotti
//

#ifndef SMTS_LIB_PROCESS_H
#define SMTS_LIB_PROCESS_H

#include "Exception.h"
#include "net.h"


class ProcessException : public Exception {
public:
    explicit ProcessException(const char *message) : ProcessException(std::string(message)) {}

    explicit ProcessException(const std::string &message) : Exception("ProcessException: " + message) {}
};


class Process {
private:
    pid_t process;
    const net::Pipe piper;
    const net::Pipe pipew;

protected:
    virtual void main() = 0;

    void start();

public:
    Process();

    virtual ~Process();

    void stop();

    void join();

    inline bool joinable() const;

    net::Socket *reader() const;

    net::Socket *writer() const;

};


#endif