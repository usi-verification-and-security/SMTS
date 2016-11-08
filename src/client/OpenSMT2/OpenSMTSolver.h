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
#include "lib/net/NetLemma.h"


namespace opensmt {
    extern bool stop;
}


class OpenSMTInterpret : public Interpret {
    friend class OpenSMTSolver;

    friend class SolverProcess;

private:
    std::map<std::string, std::string> &header;

    std::function<void(const std::vector<NetLemma> &)> lemma_push;
    std::function<void(std::vector<NetLemma> &)> lemma_pull;

protected:
    void new_solver();

public:
    OpenSMTInterpret(std::map<std::string, std::string> &header,
                     std::function<void(const std::vector<NetLemma> &)> lemma_push,
                     std::function<void(std::vector<NetLemma> &)> lemma_pull,
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

    void inline clausesPublish();

    void inline clausesUpdate();

public:
    OpenSMTSolver(OpenSMTInterpret &interpret);

    ~OpenSMTSolver();
};


#endif //CLAUSE_SERVER_OPENSMTSOLVER_H
