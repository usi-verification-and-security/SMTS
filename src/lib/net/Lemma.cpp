//
// Created by Matteo on 07/11/2016.
//

#include "lib/lib.h"
#include "Lemma.h"


namespace net {
    std::ostream &operator<<(std::ostream &stream, const std::vector<Lemma> &lemmas) {
        for (auto &lemma:lemmas) {
            stream << lemma << '\0';
        }
        return stream;
    }

    std::istream &operator>>(std::istream &stream, std::vector<Lemma> &lemmas) {
        ::split(stream, '\0', [&](const std::string &sub) {
            if (sub.size() == 0)
                return;
            lemmas.push_back(Lemma(sub));
        });
        return stream;
    }
}