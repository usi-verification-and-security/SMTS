//
// Created by Matteo on 02/12/2016.
//

#ifndef CLAUSE_SERVER_HEADER_H
#define CLAUSE_SERVER_HEADER_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <sstream>


namespace net {
    typedef std::string header_prefix;

    class Header : public std::map<std::string, std::string> {
        friend std::ostream &operator<<(std::ostream &stream, const Header &header);

        friend std::istream &operator>>(std::istream &, Header &);

    public:
        uint8_t level() const;

        net::Header copy(const std::vector<const std::string> &) const;

        net::Header copy(const header_prefix &, const std::vector<const std::string> &) const;

        const std::vector<const std::string> keys(const header_prefix &) const;

        const std::string &get(const header_prefix &, const std::string &) const;

        void set(const header_prefix &, const std::string &, const std::string &);

        void remove(const header_prefix &, const std::string &);

        static const header_prefix statistic;
        static const header_prefix parameter;

    };
}

#endif //CLAUSE_SERVER_HEADER_H
