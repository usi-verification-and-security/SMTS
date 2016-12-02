//
// Created by Matteo on 02/12/2016.
//

#ifndef CLAUSE_SERVER_HEADER_H
#define CLAUSE_SERVER_HEADER_H

#include <map>
#include <string>

namespace net {
    class Header : public std::map<std::string, std::string> {

    };

    std::ostream &pprint(std::ostream &, const Header &, bool json = false);
}

#endif //CLAUSE_SERVER_HEADER_H
