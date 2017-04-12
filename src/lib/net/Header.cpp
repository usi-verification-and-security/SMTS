//
// Created by Matteo on 02/12/2016.
//

#include <array>
#include <vector>
#include <iomanip>
#include "lib/lib.h"
#include "Header.h"


namespace net {
    std::istream &operator>>(std::istream &stream, Header &header) {
        char c;
        do {
            stream.get(c);
        } while (c && c != '{');
        if (!c)
            return stream;
        std::pair<std::string, std::string> pair;
        std::string *s = &pair.first;
        bool escape = false;
        while (stream.get(c)) {
            if (!escape && s->size() == 0 && c == ' ')
                continue;
            if (!escape && s->size() == 0) {
                if (c == '}' && s == &pair.first)
                    break;
                if (c == '"') {
                    if (!stream.get(c))
                        throw Exception("unexpected end");
                } else
                    throw Exception("double quotes expected");
            }
            if (!escape) {
                switch (c) {
                    case '\\':
                        escape = true;
                        continue;
                    case '"':
                        while (stream.get(c) && c == ' ') {
                        }
                        if (s == &pair.first) {
                            if (c != ':')
                                throw Exception("colon expected");
                            s = &pair.second;
                            continue;
                        } else {
                            header.insert(pair);
                            if (c == '}')
                                stream.unget();
                            else if (c != ',')
                                throw Exception("comma expected");
                            pair.first.clear();
                            pair.second.clear();
                            s = &pair.first;
                        }
                        break;
                    default:
                        if ('\x00' <= c && c <= '\x1f')
                            throw Exception("control char not allowed");
                        *s += c;
                }
            } else {
                escape = false;
                char i = 0;
                switch (c) {
                    case '"':
                    case '\\':
                    case 'b':
                    case 'f':
                    case 'n':
                    case 'r':
                    case 't':
                        *s += c;
                        break;
                    case 'u':
                        if (!(stream.get(c) && c == '0' && stream.get(c) && c == '0'))
                            throw Exception("unicode not supported");
                        for (uint8_t _ = 0; _ < 2; _++) {
                            if (!stream.get(c))
                                throw Exception("unexpected end");
                            c = (char) toupper(c);
                            if ((c < '0') || (c > 'F') || ((c > '9') && (c < 'A')))
                                throw Exception("bad hex string");
                            c -= '0';
                            if (c > 9)
                                c -= 7;
                            i = (i << 4) + c;
                        }
                        *s += i;
                        break;
                    default:
                        throw Exception("bad char after escape");
                }
            }
        }
        return stream;
    }

    std::ostream &operator<<(std::ostream &stream, const Header &header) {
        std::vector<std::string> pairs;
        for (auto &pair:header) {
            std::ostringstream ss;
            for (auto value:std::array<const std::string *, 2>({{&pair.first, &pair.second}})) {
                ss << "\"";
                for (auto &c:*value) {
                    switch (c) {
                        case '"':
                            ss << "\\\"";
                            break;
                        case '\\':
                            ss << "\\\\";
                            break;
                        case '\b':
                            ss << "\\b";
                            break;
                        case '\f':
                            ss << "\\f";
                            break;
                        case '\n':
                            ss << "\\n";
                            break;
                        case '\r':
                            ss << "\\r";
                            break;
                        case '\t':
                            ss << "\\t";
                            break;
                        default:
                            if ('\x00' <= c && c <= '\x1f') {
                                ss << "\\u"
                                   << std::hex << std::setw(4) << std::setfill('0') << (int) c;
                            } else {
                                ss << c;
                            }
                    }
                }
                ss << "\"";
                if (value == &pair.first)
                    ss << ":";
            }
            pairs.push_back(ss.str());
        }
        return ::join(stream << "{", ",", pairs) << "}";
    }

    uint8_t Header::level() {
        std::string node = (*this)["node"];
        node % std::make_pair("[", "") % std::make_pair("]", "") % std::make_pair(" ", "");
        auto v = ::split(node, ",");
        return (uint8_t) (v->size() / 2);
    }
}
