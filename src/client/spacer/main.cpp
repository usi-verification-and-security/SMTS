//
// Created by Matteo on 24/10/2016.
//

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <lib/net.h>
#include "lib/lib.h"


int main() {
    std::vector<std::string> v;
    ::split("cia\0o:ciao:ciao", ":", v);
    for (auto i:v) {
        std::cout << i << "\n";
    }

    std::cout << v << "\n";

}