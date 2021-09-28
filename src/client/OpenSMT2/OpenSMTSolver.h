//
// Author: Matteo Marescotti
//

#ifndef SMTS_CLIENT_OPENSMT2_OPENSMTSOLVER_H
#define SMTS_CLIENT_OPENSMT2_OPENSMTSOLVER_H

#include <vector>
#include <functional>
#include "PreInterpret.h"
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
//    PreInterpret* preInterpret;
    std::unique_ptr<PreInterpret> interpret;
    bool learned_push;
    sstat result;
public:
    OpenSMTSolver(net::Header &header, SMTConfig &config, string & instance, Channel& ch) :
    header(header),
    learned_push(false),
    result(Status::unknown)
    {
        interpret.reset(new PreInterpret(config , ch));
        interpret->interpFile((char *)instance.c_str());
//        interpret.reset( std::move(preInterpret));
//        interpret.reset( new Interpret(config, std::move(std::shared_ptr<MainSolver>(&preInterpret->getMainSolver())),
//                                       std::move(std::shared_ptr<Logic>(&preInterpret->getLogic()))));
    }

    void search();
    sstat getResult() const { return result; }



    MainSplitter& getMainSplitter() const {
        return (MainSplitter&) interpret->getMainSolver();
    };
};



#endif
