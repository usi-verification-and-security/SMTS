//
// Created by Matteo on 11/08/16.
//

#ifndef CLAUSE_SERVER_NODE_H
#define CLAUSE_SERVER_NODE_H

#include <string>
#include <map>
#include <list>
#include <vector>
#include "lib/net/Lemma.h"
#include "Lemma.h"


class Node {
private:
    std::map<std::string, Lemma *> index; // string used as index
public:
    std::vector<Node *> children;

    Node() {}

    ~Node() {
        for (auto &pair:this->index) {
            delete (this->index[pair.first]);
        }
        for (auto child:this->children) {
            delete (child);
        }
    }

    Lemma *get(net::Lemma &lemma) {
        return this->index.count(lemma.smtlib) ? this->index[lemma.smtlib] : nullptr;
    }

    Lemma *add_lemma(net::Lemma &lemma) {
        Lemma *r = this->get(lemma);
        if (!r) {
            r = new Lemma(lemma);
            this->index[lemma.smtlib] = r;
        }
        return r;
    }

    void fill(std::list<Lemma *> &lemmas) {
        for (auto &pair:this->index) {
            lemmas.push_back(this->index[pair.first]);
        }
    }

};

#endif //CLAUSE_SERVER_NODE_H
