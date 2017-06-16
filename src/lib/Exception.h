//
// Author: Matteo Marescotti
//

#ifndef SMTS_LIB_EXCEPTION_H
#define SMTS_LIB_EXCEPTION_H

#include <exception>
#include <iostream>


class Exception : public std::exception {
private:
    std::string msg;

public:
    explicit Exception(const char *message) :
            msg(message) {}

    explicit Exception(const std::string &message) :
            msg(message) {}

    const char *what() const throw() { return this->msg.c_str(); }
};

#endif
