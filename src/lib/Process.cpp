//
// Author: Matteo Marescotti
//

#include <csignal>
#include <unistd.h>
#include <sys/wait.h>
#include "Process.h"


Process::Process() :
        process(-1), piper(net::Pipe()), pipew(net::Pipe()) {}

Process::~Process() {
    this->stop();
    this->join();
}

void Process::start() {
    this->process = fork();
    if (this->process < 0)
        throw ProcessException(__FILE__, __LINE__, "fork error");
    if (this->process == 0) {
        this->piper.writer()->close();
        this->pipew.reader()->close();
        this->main();
        exit(0);
    } else {
        this->piper.reader()->close();
        this->pipew.writer()->close();
    }
}

void Process::stop() {
    if (this->joinable()) {
        kill(this->process, SIGKILL);
        this->join();
    }
}

void Process::join() {
    if (this->joinable()) {
        ::waitpid(this->process, nullptr, 0);
        this->process = -1;
    }
}

bool Process::joinable() const {
    return this->process > 0;
}

net::Socket *Process::reader() const {
    return (this->process == 0) ? this->piper.reader() : this->pipew.reader();
}

net::Socket *Process::writer() const {
    return (this->process == 0) ? this->pipew.writer() : this->piper.writer();
}
