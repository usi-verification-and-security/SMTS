//
// Author: Matteo Marescotti
//

#ifndef SMTS_CLIENT_FILETHREAD_H
#define SMTS_CLIENT_FILETHREAD_H

#include "lib/Thread.h"
#include "Settings.h"


class FileThread : public Thread {
private:
    Settings &settings;
    net::Socket server;

protected:
    void main();

public:
    FileThread(Settings &);

};


#endif
