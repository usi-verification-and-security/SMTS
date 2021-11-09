//
// Author: Matteo Marescotti
//

#ifndef SMTS_CLIENT_OPENSMT2_OPENSMTSOLVER_H
#define SMTS_CLIENT_OPENSMT2_OPENSMTSOLVER_H

#include <vector>
#include <functional>
#include "client/SolverProcess.h"
#include "client/Settings.h"
#include "lib/net/Lemma.h"
#include "PreInterpret.h"


namespace opensmt {
    extern bool stop;
}


class OpenSMTSolver  {
    friend class SolverProcess;

private:
    std::unique_ptr<PreInterpret> preInterpret;
    bool learned_push;
    sstat result;

public:
    OpenSMTSolver(SMTConfig &config, string & instance, Channel& channel) :
    learned_push(false),
    result(PartitionChannel::Status::unknown)
    {
        preInterpret.reset(new PreInterpret(config , channel));
        if (not channel.shouldTerminate()) {
            preInterpret->interpFile((char *)instance.c_str());
        }
//        interpret.reset( std::move(preInterpret));
//        interpret.reset( new Interpret(config, std::move(std::shared_ptr<MainSolver>(&preInterpret->getMainSolver())),
//                                       std::move(std::shared_ptr<Logic>(&preInterpret->getLogic()))));
    }

    void search();
    sstat getResult() const    { return result; }
    void setResult(sstat res)  {  result = res; }

    inline MainSplitter& getMainSplitter() const {
        return (MainSplitter&) preInterpret->getMainSolver();
    };
};



#endif
