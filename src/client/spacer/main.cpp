//
// Created by Matteo on 24/10/2016.
//

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <lib/net.h>
// #include "fixedpoint.h"
#include "lib/lib.h"


int main() {
    std::vector<std::string> v;
    ::split("ciao:ciao:ciao", ":", v,2);
    for (auto i:v) {
        std::cout << i << "\n";
    }


}