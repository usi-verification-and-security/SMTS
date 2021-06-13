//
// Author: Matteo Marescotti
//

#ifndef SMTS_CLIENT_OPENSMT2_OPENSMTSOLVER_H
#define SMTS_CLIENT_OPENSMT2_OPENSMTSOLVER_H

#include <vector>
#include <functional>
#include "Interpret.h"
#include "client/SolverProcess.h"
#include "client/Settings.h"
#include "lib/net/Lemma.h"


namespace opensmt {
    extern bool stop;
}


class OpenSMTSolver  {
    friend class SolverProcess;

private:
    net::Header &header;
    std::unique_ptr<Interpret> interpret;
    bool learned_push;

public:
    OpenSMTSolver(net::Header &header,
                     SMTConfig &c) :
            interpret(new Interpret(c)),
            header(header),
            learned_push(false)  {
    }

};



#endif
