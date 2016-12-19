//
// Created by Matteo on 02/12/2016.
//

#ifndef CLAUSE_SERVER_HEADER_H
#define CLAUSE_SERVER_HEADER_H

#include <map>
#include <string>
#include <sstream>


namespace net {
    class Header : public std::map<std::string, std::string> {
        friend std::ostream &operator<<(std::ostream &stream, const Header &header);

        friend std::istream &operator>>(std::istream &, Header &);

    };

}

#endif //CLAUSE_SERVER_HEADER_H
