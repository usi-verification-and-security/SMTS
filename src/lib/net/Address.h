//
// Created by Matteo on 12/08/16.
//

#ifndef CLAUSE_SERVER_ADDRESS_H
#define CLAUSE_SERVER_ADDRESS_H

#include <string>
#include <sstream>


namespace net {
    class Address {
    public:
        Address(std::string);

        Address(std::string, uint16_t);

        Address(struct sockaddr_storage *);

        friend std::ostream &operator<<(std::ostream &stream, const Address &address) {
            return stream << address.hostname << ":" << std::to_string(address.port);
        }

        std::string hostname;
        uint16_t port;
    };
}


#endif //CLAUSE_SERVER_ADDRESS_H
