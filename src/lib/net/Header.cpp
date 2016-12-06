//
// Created by Matteo on 02/12/2016.
//

#include "Header.h"
#include "lib/lib.h"


namespace net {

    //std::istream &operator>>(std::istream &stream, Header &header) {
//        stream >> lemma.level;
//        lemma.smtlib = std::string(std::istreambuf_iterator<char>(stream), {});
//        return stream;
    // }

    std::ostream &operator<<(std::ostream &stream, const Header &header) {
        stream << "{";
        std::vector<std::string> items;
        for (auto &pair:header) {
            std::string first(pair.first);
            std::string second(pair.second);
            replace(first, "\"", "\\\"");
            replace(second, "\"", "\\\"");
            items.push_back("\"" + first + "\":\"" + second + "\"");
        }
        join(stream, ",", items);
        stream << "}";
        return stream;
    }

//    std::ostream &Header::format(std::ostream &stream, bool indent) const {
//        stream << "{";
//        if (indent)
//            stream << "\n";
//        if (indent) {
//            for (auto &pair:*this) {
//                stream << "  " << pair.first << ":\t" << pair.second << "\n";
//            }
//        } else {
//            std::vector<std::string> items;
//            for (auto &pair:*this) {
//                std::string first(pair.first);
//                std::string second(pair.second);
//                replace(first, "\"", "\\\"");
//                replace(second, "\"", "\\\"");
//                items.push_back(
//                        std::string(std::string("\"") + first + std::string("\":\"") + second + std::string("\""))
//                );
//            }
//            join(stream, ",", items);
//
//        }
//        stream << "}";
//        if (indent)
//            stream << "\n";
//        return stream;
//    }
}
