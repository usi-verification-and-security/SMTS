//
// Created by Matteo on 12/08/16.
//

#include <signal.h>
#include <unistd.h>
#include "Pipe.h"


namespace net {
    Pipe::Pipe(int r, int w) :
            r(new Socket(r)),
            w(new Socket(w)) {
        signal(SIGPIPE, SIG_IGN);
    }

    Pipe::Pipe() {
        int fd[2];
        if (::pipe(fd) == -1)
            throw SocketException("pipe creation error");
        new(this) Pipe(fd[0], fd[1]);
    }

    Pipe::~Pipe() {
        delete this->r;
        delete this->w;
    }

    Socket *Pipe::reader() const {
        return this->r;
    }

    Socket *Pipe::writer() const {
        return this->w;
    }
}