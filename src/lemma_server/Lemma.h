//
// Author: Matteo Marescotti
//

#ifndef SMTS_LEMMASERVER_LEMMA_H
#define SMTS_LEMMASERVER_LEMMA_H

#include <string>
#include <list>
#include "lib/net/Lemma.h"


class Lemma : public net::Lemma {
private:
    int score;
public:
    Lemma(net::Lemma &lemma) :
            net::Lemma(lemma),
            score(0),
            id(0) {}

    static bool compare(const Lemma *const &a, Lemma *const &b) {
        return a->score < b->score;
    }

    void increase() { //this->score = (this->score + 1) - this->score == 1 ? this->score + 1 : this->score;
        this->score++;
    }

    void decrease() { this->score--; }

    inline int get_score() { return this->score; }

    bool operator==(Lemma const &b) const { return this->smtlib == b.smtlib; }

    bool operator!=(Lemma const &b) const { return this->smtlib != b.smtlib; }

    bool operator<(Lemma const &b) const { return this->score < b.score; }

    bool operator>(Lemma const &b) const { return this->score > b.score; }

    bool operator<=(Lemma const &b) const { return this->score <= b.score; }

    bool operator>=(Lemma const &b) const { return this->score >= b.score; }

    uint32_t id;
};


#endif
