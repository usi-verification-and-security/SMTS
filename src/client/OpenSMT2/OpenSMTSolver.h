//
// Created by Matteo on 22/07/16.
//

#ifndef CLAUSE_SERVER_OPENSMTSOLVER_H
#define CLAUSE_SERVER_OPENSMTSOLVER_H

#include <vector>
#include <functional>
#include "SimpSMTSolver.h"
#include "Interpret.h"
#include "client/SolverProcess.h"
#include "client/Settings.h"
#include "lib/net/Lemma.h"


namespace opensmt {
    extern bool stop;
}


class OpenSMTInterpret : public Interpret {
    friend class OpenSMTSolver;

    friend class SolverProcess;

private:
    net::Header &header;

    std::function<void(const std::vector<net::Lemma> &)> lemma_push;
    std::function<void(std::vector<net::Lemma> &)> lemma_pull;

protected:
    void new_solver();

public:
    OpenSMTInterpret(net::Header &header,
                     std::function<void(const std::vector<net::Lemma> &)> lemma_push,
                     std::function<void(std::vector<net::Lemma> &)> lemma_pull,
                     SMTConfig &c) :
            Interpret(c),
            header(header),
            lemma_push(lemma_push),
            lemma_pull(lemma_pull) {}

};

class OpenSMTSolver : public SimpSMTSolver {
private:
    OpenSMTInterpret &interpret;
    uint32_t trail_sent;
    bool learned_push;

    void inline clausesPublish();

    void inline clausesUpdate();

public:
    OpenSMTSolver(OpenSMTInterpret &interpret);

    ~OpenSMTSolver();
};


#endif //CLAUSE_SERVER_OPENSMTSOLVER_H
