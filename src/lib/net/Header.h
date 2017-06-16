//
// Author: Matteo Marescotti
//

#ifndef SMTS_LIB_NET_HEADER_H
#define SMTS_LIB_NET_HEADER_H

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

        net::Header copy(const std::vector<std::string> &) const;

        net::Header copy(const header_prefix &, const std::vector<std::string> &) const;

        const std::vector<std::string> keys(const header_prefix &) const;

        const std::string &get(const header_prefix &, const std::string &) const;

        void set(const header_prefix &, const std::string &, const std::string &);

        void remove(const header_prefix &, const std::string &);

        static const header_prefix statistic;
        static const header_prefix parameter;

    };
}

#endif
