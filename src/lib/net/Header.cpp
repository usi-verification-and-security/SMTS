//
// Created by Matteo on 02/12/2016.
//

#include "Header.h"

namespace net {
    std::ostream &pprint(std::ostream &stream, const Header &map, bool json) {
        stream << "{";
        if (!json)
            stream << "\n";
        if (json) {
            std::vector<std::string> items;
            for (auto &pair:map) {
                std::string first(pair.first);
                std::string second(pair.second);
                replace(first, "\"", "\\\"");
                replace(second, "\"", "\\\"");
                items.push_back(
                        std::string(std::string("\"") + first + std::string("\": \"") + second + std::string("\""))
                );
            }
            join(stream, ", ", items);
        } else
            for (auto &pair:map) {
                stream << "  " << pair.first << ": " << pair.second << "\n";
            }
        stream << "}";
        if (!json)
            stream << "\n";
        return stream;
    }
}
