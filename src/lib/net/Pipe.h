//
// Author: Matteo Marescotti
//

#ifndef SMTS_LIB_NET_PIPE_H
#define SMTS_LIB_NET_PIPE_H

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


#endif
