//
// Created by Matteo on 07/11/2016.
//

#ifndef CLAUSE_SERVER_LEMMA_H
#define CLAUSE_SERVER_LEMMA_H

#include <vector>
#include <sstream>


namespace net {
    class Lemma {
    public:
        friend std::ostream &operator<<(std::ostream &stream, const Lemma &lemma) {
            return stream << std::to_string(lemma.level) << " " << lemma.smtlib;
        }

        friend std::istream &operator>>(std::istream &stream, Lemma &lemma) {
            stream >> lemma.level;
            lemma.smtlib = std::string(std::istreambuf_iterator<char>(stream), {});
            return stream;
        }

        friend std::ostream &operator<<(std::ostream &stream, const std::vector<Lemma> &lemmas);

        friend std::istream &operator>>(std::istream &stream, std::vector<Lemma> &lemmas);

        Lemma(const std::string &smtlib, const uint8_t level) : smtlib(smtlib), level(level) {}

        Lemma(const std::string &dump) {
            std::istringstream is(dump);
            is >> *this;
        }

        std::string smtlib;
        uint16_t level;
    };
}

#endif //CLAUSE_SERVER_LEMMA_H
