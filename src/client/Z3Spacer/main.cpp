//
// Author: Matteo Marescotti
//

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <vector>
#include "lib/lib.h"


net::Header a(){
    net::Header h;
    h["parameter.a"] = "A";
    h["parameter.b"] = "B";
    h["asd"] = "aasd";
    std::cout << &h<< "\n";
    return h;
}

int main() {
    net::Header h=a();
    std::cout << &h << "\n";
    auto c = ::split("ciao,ciao,ciao",",");
    h["o"]=to_string(c);
    std::cout << h << "\n";
//    std::cout << h << "\n";
//
// Author: Matteo Marescotti
//    std::cout << h.copy(net::Header::parameter, h.keys(net::Header::parameter)) << "\n";
//    std::cout << h.copy({"asd"}) << "\n";

    std::cout << &h["parameter.a"] << "\n";
    std::cout << &h.get(net::Header::parameter, "a") << "\n";

}