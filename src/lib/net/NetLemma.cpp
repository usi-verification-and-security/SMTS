//
// Created by Matteo on 07/11/2016.
//

#include "lib/lib.h"
#include "NetLemma.h"


NetLemma::NetLemma(const std::string &dump) {
    std::vector<std::string> l_s;
    ::split(dump, " ", l_s, 2);
    if (l_s.size() != 2)
        throw Exception("badly formatted NetLemma");
    new(this) NetLemma(l_s[1], (uint8_t) std::stoul(l_s[0]));
}
