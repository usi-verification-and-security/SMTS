//
// Created by Matteo on 12/08/16.
//

#ifndef CLAUSE_SERVER_PIPE_H
#define CLAUSE_SERVER_PIPE_H

#include "Socket.h"


namespace net {
    class Pipe {
    private:
        Socket *r;
        Socket *w;

        Pipe(int, int);

    public:
        Pipe();

        ~Pipe();

        Socket *reader() const;

        Socket *writer() const;

    };
}


#endif //CLAUSE_SERVER_PIPE_H
