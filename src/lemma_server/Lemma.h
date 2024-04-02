/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMTS_LEMMASERVER_LEMMA_H
#define SMTS_LEMMASERVER_LEMMA_H

#include <PTPLib/net/Lemma.hpp>

#include <string>
#include <list>
#include <cstdint>

class Lemma : public PTPLib::net::Lemma {
private:
    int score;

public:
    Lemma(PTPLib::net::Lemma &lemma)
    :
        PTPLib::net::Lemma(lemma),
        score(0),
        id(0)
    {}

    static bool score_compare(const Lemma *const &a, Lemma *const &b) {
        return a->score < b->score;
    }

    static bool level_compare(const Lemma *const &a, Lemma *const &b) {
        return a->level < b->level;
    }

    void increase() { //this->score = (this->score + 1) - this->score == 1 ? this->score + 1 : this->score;
        this->score++;
    }

    void decrease() { this->score--; }

    inline int get_score() { return this->score; }

    bool operator==(Lemma const &b) const { return this->clause == b.clause; }

    bool operator!=(Lemma const &b) const { return this->clause != b.clause; }

    bool operator<(Lemma const &b) const { return this->score < b.score; }

    bool operator>(Lemma const &b) const { return this->score > b.score; }

    bool operator<=(Lemma const &b) const { return this->score <= b.score; }

    bool operator>=(Lemma const &b) const { return this->score >= b.score; }

    uint32_t id;
};


#endif
