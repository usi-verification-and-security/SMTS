//
// Created by Matteo on 07/11/2016.
//

#ifndef CLAUSE_SERVER_LEMMA_H
#define CLAUSE_SERVER_LEMMA_H

#include <vector>
#include <sstream>


class NetLemma {
public:
    friend std::ostream &operator<<(std::ostream &stream, const NetLemma &lemma) {
        return stream << std::to_string(lemma.level) << " " << lemma.smtlib;
    }

    NetLemma(const std::string &smtlib, const uint8_t level) : smtlib(smtlib), level(level) {}

    NetLemma(const std::string &dump);

    const std::string smtlib;
    uint8_t level;
};

#endif //CLAUSE_SERVER_LEMMA_H
