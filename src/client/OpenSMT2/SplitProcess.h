//
// Created by Matteo on 07/09/16.
//

#ifndef CLAUSE_SERVER_SPLITPROCESS_H
#define CLAUSE_SERVER_SPLITPROCESS_H

#include "lib/Process.h"
#include "client/SolverProcess.h"
#include "OpenSMTSolver.h"


class SplitProcess : public Process {
private:
    MainSolver *solver;
    Task task;
protected:
    void main();

public:
    SplitProcess(MainSolver *, Task &);
};


#endif //CLAUSE_SERVER_SPLITPROCESS_H
