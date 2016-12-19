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
    net::Header h;
    std::istringstream is("{\"a\":\"A\" ,\"b\":\"B\"}");
    is >> h;
    std::cout << h << "\n";
}