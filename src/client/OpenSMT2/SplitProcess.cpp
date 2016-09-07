//
// Created by Matteo on 07/09/16.
//

#include <map>
#include <string>
#include "SplitProcess.h"


SplitProcess::SplitProcess(MainSolver *solver, Task &task) :
        solver(solver),
        task(task) {
    this->start();
}

void SplitProcess::main() {
    std::map<std::string, std::string> header;
    std::string payload;
    const char *msg;
    if (!(
            this->solver->getConfig().setOption(SMTConfig::o_sat_split_num, SMTOption(int(this->task.partitions)),
                                                msg) &&
            this->solver->getConfig().setOption(SMTConfig::o_sat_split_type, SMTOption(spts_lookahead), msg) &&
            this->solver->getConfig().setOption(SMTConfig::o_sat_split_units, SMTOption(spts_time), msg) &&
            this->solver->getConfig().setOption(SMTConfig::o_sat_split_inittune, SMTOption(double(2)), msg) &&
            this->solver->getConfig().setOption(SMTConfig::o_sat_split_midtune, SMTOption(double(2)), msg) &&
            this->solver->getConfig().setOption(SMTConfig::o_sat_split_asap, SMTOption(1), msg))) {
        header["error"] = msg;
    }
    else {
        sstat status = this->solver->solve();
        if (status == s_Undef) {
            vec<SplitData> &splits = this->solver->getSMTSolver().splits;
            for (int i = 0; i < splits.size(); i++) {
                vec<vec<PtAsgn>> constraints;
                splits[i].constraintsToPTRefs(constraints);
                vec<PTRef> clauses;
                for (int j = 0; j < constraints.size(); j++) {
                    vec<PTRef> clause;
                    for (int k = 0; k < constraints[j].size(); k++) {
                        PTRef pt =
                                constraints[j][k].sgn == l_True ?
                                constraints[j][k].tr :
                                this->solver->getLogic().mkNot(constraints[j][k].tr);
                        clause.push(pt);
                    }
                    clauses.push(this->solver->getLogic().mkOr(clause));
                }
                char *str = this->solver->getTHandler().getLogic().
                        printTerm(this->solver->getLogic().mkAnd(clauses), false, true);
                payload += str;
                payload += "\n";
                free(str);
            }
            header["partitions"] = std::to_string(splits.size());
        }
        else if (status == s_True)
            header["status"] = (uint8_t) Status::sat;
        else if (status == s_False)
            header["status"] = (uint8_t) Status::unsat;
        else
            header["error"] = "unknown status after partition";
    }
    this->writer()->write(header, payload);
}
