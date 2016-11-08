//
// Created by Matteo on 11/08/16.
//

#include "lib.h"


void split(const std::string &string, const std::string &delimiter, std::vector<std::string> &vector, uint32_t limit) {
    return split(string, delimiter, [&vector](const std::string &sub) {
        vector.push_back(sub);
    }, limit);
}

void split(const std::string &string,
           const std::string &delimiter,
           std::function<void(const std::string &)> callback,
           uint32_t limit) {
    size_t b = 0;
    size_t e;
    std::string sub;
    while (true) {
        if (limit != 0 && --limit == 0)
            e = std::string::npos;
        else
            e = string.find(delimiter, b);
        callback(string.substr(b, e - b));
        if (e == std::string::npos)
            return;
        b = e + delimiter.size();
    }
}

void replace(std::string &string, const std::string &from, const std::string &to) {
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = string.find(from, start_pos)) != std::string::npos) {
        string.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}
