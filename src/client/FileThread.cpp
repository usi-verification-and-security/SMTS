//
// Created by Matteo on 22/07/16.
//

#include <iostream>
#include <fstream>
#include "lib/Logger.h"
#include "FileThread.h"


FileThread::FileThread(Settings &settings) :
        settings(settings),
        server((uint16_t) 0) {
    if (settings.server.size())
        throw Exception("server must be null");
    settings.server = this->server.get_local().toString();
    this->start();
}

void FileThread::main() {
    std::map<std::string, std::string> header;

    std::shared_ptr<net::Socket> client(this->server.accept());
    std::shared_ptr<net::Socket> lemmas;

    if (this->settings.lemmas.size()) {
        try {
            lemmas.reset(new net::Socket(this->settings.lemmas));
        } catch (net::SocketException) {
        }
        header["command"] = "lemmas";
        header["lemmas"] = this->settings.lemmas;
        client->write(header, "");
    }

    for (auto &filename : this->settings.files) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            Logger::log(Logger::WARNING, "unable to open: " + filename);
            continue;
        }

        std::string payload;
        file.seekg(0, std::ios::end);
        payload.resize((unsigned long) file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(&payload[0], payload.size());
        file.close();

        header.clear();
        for (auto &it : this->settings.header_solve)
            header[it.first] = it.second;
        header["command"] = "solve";
        header["name"] = filename;
        header["node"] = "[]";
        client->write(header, payload);
        do {
            client->read(header, payload);
        } while (header.count("status") == 0 && header.count("error") == 0);
        if (lemmas)
            try {
                header["lemmas"] = std::string("0");
                lemmas->write(header, "");
            } catch (net::SocketException) {
                lemmas.reset();
            }
        header["command"] = "stop";
        header["name"] = filename;
        header["node"] = "[]";
        client->write(header, "");
    }
}