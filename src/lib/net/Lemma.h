//
// Author: Matteo Marescotti
//

#ifndef SMTS_LIB_NET_LEMMA_H
#define SMTS_LIB_NET_LEMMA_H

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

        Lemma(const std::string &smtlib, const uint16_t level) : smtlib(smtlib), level(level) {}

        Lemma() : Lemma("", 0) {}

        std::string smtlib;
        int level;
    };
}

#endif
