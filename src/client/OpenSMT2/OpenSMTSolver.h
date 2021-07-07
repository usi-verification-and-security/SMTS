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
    sstat result;
public:
    OpenSMTSolver(net::Header &header, SMTConfig &config, char* instance) :
            header(header),
            learned_push(false),
            result(Status::unknown)

    {
        interpret.reset( new Interpret(config));
        config.SMTConfig::o_smts_check_sat_ON = false;
        config.SMTConfig::o_smts_mainSplitter_ON = true;
        interpret->interpFile(instance);
    }
    void search();
    sstat getResult() const { return result; }

    Channel& getChannel() const {
        //assert(mainSolver->getChannel());
       return ( (MainSplitter&) interpret->getMainSolver() ).getChannel();
    };

    MainSplitter& getMainSplitter() const {
        return (MainSplitter&) interpret->getMainSolver();
    };
};



#endif
