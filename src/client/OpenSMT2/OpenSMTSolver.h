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
    std::function<void(const std::vector<net::Lemma> &)> lemma_push;
    std::function<void(std::vector<net::Lemma> &)> lemma_pull;
    uint32_t trail_sent;
    bool learned_push;

public:
    OpenSMTSolver(net::Header &header,
                     std::function<void(const std::vector<net::Lemma> &)> lemma_push,
                     std::function<void(std::vector<net::Lemma> &)> lemma_pull,
                     SMTConfig &c) :
            interpret(new Interpret(c)),
            header(header),
            lemma_push(lemma_push),
            lemma_pull(lemma_pull),
            trail_sent(0),
            learned_push(false)  {
    }

};



#endif
