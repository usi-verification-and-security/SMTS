//
// Created by Matteo Marescotti on 02/12/15.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include "lib/lib.h"
#include "lib/Log.h"
#include "lib/Exception.h"
#include "LemmaServer.h"


LemmaServer::LemmaServer(Settings &settings) :
        Server(settings.port),
        settings(settings),
        server(nullptr),
        db(nullptr) {
    if (this->settings.server.port > 0) {
        this->server = new Socket(this->settings.server);
        std::map<std::string, std::string> header;
        header["lemmas"] = ":" + std::to_string(this->settings.port);
        this->server->write(header, "");
        this->add_socket(this->server);
    }
    if (this->settings.db_filename.size() > 0) {
        this->db = new SQLite3(this->settings.db_filename);
        this->db->exec("CREATE TABLE IF NOT EXISTS Lemma"
                               "(id INTEGER PRIMARY KEY, name TEXT, node TEXT, score INTEGER, smtlib TEXT);");
    }
};

LemmaServer::~LemmaServer() {
    if (this->db)
        delete this->db;
}


void LemmaServer::handle_accept(Socket &client) {
    Log::log(Log::INFO, "+ " + client.get_remote().toString());
}


void LemmaServer::handle_close(Socket &client) {
    if (&client == this->server)
        throw Exception("server connection closed.");
    for (auto &pair:this->solvers) {
        if (this->solvers[pair.first].count(client.get_remote().toString())) {
            this->solvers[pair.first].erase(client.get_remote().toString());
        }
    }
    Log::log(Log::INFO, "- " + client.get_remote().toString());
}


void LemmaServer::handle_exception(Socket &client, SocketException &ex) {
    Log::log(Log::ERROR, "Exception from: " + client.get_remote().toString() + ": " + ex.what());
}


void LemmaServer::handle_message(Socket &client,
                                 std::map<std::string, std::string> &header,
                                 std::string &payload) {
    if (header.count("name") == 0 || header.count("node") == 0 || header.count("lemmas") == 0)
        return;

    if (this->lemmas.count(header["name"]) != 1) {
        this->lemmas[header["name"]] = new Node;
    }

    if (header["node"].size() < 2)
        return;

    uint32_t clauses_request = 0;
    try {
        clauses_request = (uint32_t) stoi(header["lemmas"]);
    } catch (std::invalid_argument &ex) {
        return;
    }

    std::vector<Node *> node_path;
    node_path.push_back(this->lemmas[header["name"]]);
    try {
        ::split(header["node"].substr(1, header["node"].size() - 2), ",", [&node_path](const std::string &node_index) {
            int index;
            index = stoi(node_index);
            if (index < 0)
                throw std::invalid_argument("negative index");
            while ((unsigned) index >= node_path.back()->children.size()) {
                node_path.back()->children.push_back(new Node);
            }
            node_path.push_back(node_path.back()->children[index]);
        });
    } catch (std::invalid_argument &ex) {
        return;
    }

    if (clauses_request == 0) {
        if (!this->settings.clear_lemmas)
            return;
        Log::log(Log::INFO,
                 header["name"] + header["node"] + " " + client.get_remote().toString() +
                 " clear");
        Node *node_remove = node_path.back();
        node_path.pop_back();
        if (node_path.size() > 0) {
            std::replace(node_path.back()->children.begin(),
                         node_path.back()->children.end(),
                         node_remove,
                         new Node);
        } else {
            this->lemmas.erase(header["name"]);
        }
        delete node_remove;
        return;
    }

    // push
    if (header.count("separator") == 1) {
        //std::list<Lemma *> *lemmas = &node_path.back()->lemmas;
        std::list<Lemma *> *lemmas_solver = &this->solvers[header["name"]][client.get_remote().toString()];

        uint32_t pushed = 0;
        uint32_t n = 0;
        split(payload, header["separator"], [&](const std::string &lemma_dump) {
            if (lemma_dump.size() == 0)
                return;
            NetLemma netlemma(lemma_dump);
            if (netlemma.smtlib.size() == 0)
                return;
            // level is too deep, some more push involved somehow
            if (netlemma.level >= node_path.size())
                return;
            std::list<Lemma *> &node_lemmas = node_path[netlemma.level]->lemmas;
            auto it = std::find_if(node_lemmas.begin(), node_lemmas.end(), [&netlemma](const Lemma *other) {
                return other->smtlib == netlemma.smtlib;
            });
            if (it != node_lemmas.end()) {
                (*it)->increase();
                lemmas_solver->push_back(*it);
                if (this->db) {
                    SQLite3Stmt stmt = *this->db->prepare(
                            "UPDATE Lemma SET score=? WHERE name=? AND node=? AND smtlib=?;");
                    stmt.bind(1, (*it)->get_score());
                    stmt.bind(2, header["name"].c_str(), -1);
                    stmt.bind(3, header["node"].c_str(), -1);
                    stmt.bind(4, (*it)->smtlib.c_str(), -1);
                    stmt.exec();
                }
            } else {
                Lemma *lemma = new Lemma(netlemma.smtlib);
                pushed++;
                node_lemmas.push_back(lemma);
                lemmas_solver->push_back(node_lemmas.back());
                if (this->db) {
                    SQLite3Stmt stmt = *this->db->prepare(
                            "INSERT INTO Lemma (name,node,score,smtlib) VALUES(?,?,?,?);");
                    stmt.bind(1, header["name"].c_str(), -1);
                    stmt.bind(2, header["node"].c_str(), -1);
                    stmt.bind(3, lemma->get_score());
                    stmt.bind(4, lemma->smtlib.c_str(), -1);
                    stmt.exec();
                }
            }
            n++;
        });

        Log::log(Log::INFO,
                 header["name"] + header["node"] + " " + client.get_remote().toString() +
                 " push [" + std::to_string(clauses_request) + "]\t" +
                 std::to_string(n) +
                 "\t(" + std::to_string(pushed) + "\tfresh, " + std::to_string(n - pushed) + "\tpresent)");

    } else {
        std::list<Lemma *> *lemmas = new std::list<Lemma *>();
        std::list<Lemma *> *lemmas_solver = nullptr;

        if (header.count("exclude") && this->solvers[header["name"]].count(header["exclude"])) {
            lemmas_solver = &this->solvers[header["name"]][header["exclude"]];
        }

        for (auto node:node_path) {
            lemmas->merge(std::list<Lemma *>(node->lemmas));
        }
        lemmas->sort(Lemma::compare);

        header["separator"] = "\n";

        std::stringstream lemmas_dump;
        uint32_t n = 0;
        for (auto lemma = lemmas->rbegin(); lemma != lemmas->rend(); ++lemma) {
            if (n >= clauses_request)
                break;
            if (lemmas_solver != nullptr) {
                auto it = std::find_if(lemmas_solver->begin(), lemmas_solver->end(), [&lemma](const Lemma *other) {
                    return other->smtlib == (*lemma)->smtlib;
                });
                if (it != lemmas_solver->end())
                    continue;
                lemmas_solver->push_back(*lemma);
            }
            lemmas_dump << NetLemma((*lemma)->smtlib, 0) << header["separator"];
            n++;
        }

        header["lemmas"] = std::to_string(n);
        try {
            client.write(header, lemmas_dump.str());
        }
        catch (SocketException ex) { return; }
        Log::log(Log::INFO,
                 header["name"] + header["node"] + " " + client.get_remote().toString() +
                 " pull [" + std::to_string(clauses_request) + "]\t" +
                 std::to_string(n));
    }
}


