//
// Author: Matteo Marescotti
//

#include "lib/lib.h"
#include "OpenSMTSolver.h"


void OpenSMTInterpret::new_solver() {
    this->solver = new OpenSMTSolver(*this);
}


OpenSMTSolver::OpenSMTSolver(OpenSMTInterpret &interpret) :
        SimpSMTSolver(interpret.config, *interpret.thandler),
        interpret(interpret),
        trail_sent(0),
        learned_push(false) {}

OpenSMTSolver::~OpenSMTSolver() {}

void inline OpenSMTSolver::clausesPublish() {
    if (this->interpret.lemma_push == nullptr)
        return;

    std::vector<net::Lemma> lemmas;

    int trail_max = this->trail_lim.size() == 0 ? this->trail.size() : this->trail_lim[0];
    for (int i = this->trail_sent; i < trail_max; i++) {
        this->trail_sent++;
        PTRef pt = this->interpret.thandler->varToTerm(var(this->trail[i]));
        pt = sign(this->trail[i]) ? this->interpret.logic->mkNot(pt) : pt;
        char *s = this->interpret.thandler->getLogic().printTerm(pt, false, true);
        lemmas.push_back(net::Lemma(s, 0));
        free(s);
    }

    std::vector<Var> enabled_assumptions;
    for (int i = 0; i < this->assumptions.size(); i++) {
        if (sign(this->assumptions[i]))
            enabled_assumptions.push_back(var(this->assumptions[i]));
    }

    for (int i = 0; i < this->learnts.size(); i++) {
        CRef cr = this->learnts[i];
        Clause &c = this->ca[cr];
        if (c.size() > 3 || c.mark() == 3)
            continue;
        uint16_t level = 0;
        vec<PTRef> clause;
        for (int j = 0; j < c.size(); j++) {
            Lit &l = c[j];
            uint8_t k;
            for (k = 0; k < enabled_assumptions.size(); k++) {
                if (var(l) == enabled_assumptions[k]) {
                    if (level <= k)
                        level = (uint16_t) (k + 1);
                    break;
                }
            }
            if (k != enabled_assumptions.size())
                continue;
            PTRef pt = this->interpret.thandler->varToTerm(var(l));
            pt = sign(l) ? this->interpret.logic->mkNot(pt) : pt;
            clause.push(pt);
        }
        if (clause.size() == 0)
            continue;
        PTRef pt = this->interpret.logic->mkOr(clause);
        char *s = this->interpret.thandler->getLogic().printTerm(pt, false, true);
        lemmas.push_back(net::Lemma(s, level));
        free(s);
        c.mark(3);
    }
    this->interpret.lemma_push(lemmas);
}

void inline OpenSMTSolver::clausesUpdate() {
    if (this->interpret.lemma_pull == nullptr)
        return;

    std::vector<net::Lemma> lemmas;

    this->interpret.lemma_pull(lemmas);

    if (lemmas.size() == 0)
        return;

    if (this->learned_push)
        this->interpret.main_solver->pop();

    this->interpret.main_solver->push();
    this->learned_push = true;

    for (auto &lemma:lemmas) {
        if (lemma.smtlib.size() > 0)
            this->interpret.interpFile((char *) ("(assert " + lemma.smtlib + ")").c_str());
    }
}
