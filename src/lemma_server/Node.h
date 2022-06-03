/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMTS_LEMMASERVER_NODE_H
#define SMTS_LEMMASERVER_NODE_H

#include "Lemma.h"

#include <PTPLib/net/Lemma.hpp>

#include <string>
#include <map>
#include <list>
#include <vector>
#include <unordered_map>


class Node {
private:
    std::map<std::string, Lemma *> index; // string used as index
public:
    std::vector<Node *> children;

    Node() {}

    ~Node() {
        for (auto const & pair : this->index) {
            delete (this->index[pair.first]);
        }
        for (auto const & child : this->children) {
            delete (child);
        }
    }

    Lemma *get(PTPLib::net::Lemma &lemma) {
        return this->index.count(lemma.clause) ? this->index[lemma.clause] : nullptr;
    }

    Lemma *add_lemma(PTPLib::net::Lemma &lemma) {
        Lemma *r = this->get(lemma);
        if (!r) {
            r = new Lemma(lemma);
            this->index[lemma.clause] = r;
        }
        return r;
    }

    void filter(std::vector<Lemma *> &lemmas, std::unordered_map<Lemma *, bool> &lemmas_solver) {
        for (auto const & pair : this->index) {
            if (lemmas_solver[this->index[pair.first]])
                continue;
            lemmas.push_back(this->index[pair.first]);
        }
    }

};

#endif
