//
// Created by Matteo Marescotti on 02/12/15.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <string>
#include <algorithm>
#include "lib/lib.h"
#include "lib/Logger.h"
#include "lib/Exception.h"
#include "LemmaServer.h"


LemmaServer::LemmaServer(uint16_t port, const std::string &server, const std::string &db_filename) :
        Server(port) {
    if (server.size()) {
        this->server.reset(new net::Socket(server));
        net::Header header;
        header["lemmas"] = ":" + std::to_string(port);
        this->server->write(header, "");
        this->add_socket(this->server);
    }
    if (db_filename.size()) {
        this->db.reset(new SQLite3::Connection(db_filename));
        this->db->exec("CREATE TABLE IF NOT EXISTS Push("
                               "id INTEGER PRIMARY KEY, "
                               "ts INTEGER, "
                               "name TEXT, "
                               "node TEXT, "
                               "data TEXT"
                               ");");
        this->db->exec("CREATE TABLE IF NOT EXISTS Lemma("
                               "id INTEGER PRIMARY KEY, "
                               "pid INTEGER REFERENCES Push(id), "
                               "level INTEGER, "
                               "score INTEGER, "
                               "smtlib TEXT"
                               ");");
    }
};

void LemmaServer::handle_accept(net::Socket &client) {
    Logger::log(Logger::INFO, "+ " + to_string(client.get_remote()));
}

void LemmaServer::handle_close(net::Socket &client) {
    Logger::log(Logger::INFO, "- " + to_string(client.get_remote()));
    if (&client == this->server.get()) {
        Logger::log(Logger::INFO, "server connection closed. ");
        this->stop();
        return;
    }
    for (auto &pair:this->solvers) {
        if (this->solvers[pair.first].count(&client)) {
            this->solvers[pair.first].erase(&client);
        }
    }
}

void LemmaServer::handle_exception(net::Socket &client, net::SocketException &ex) {
    Logger::log(Logger::ERROR, "Exception from: " + to_string(client.get_remote()) + ": " + ex.what());
}

void LemmaServer::handle_message(net::Socket &client,
                                 net::Header &header,
                                 std::string &payload) {
    //pprint(header);
    if (header.count("name") == 0 || header.count("node") == 0 || header.count("lemmas") == 0)
        return;

    if (this->lemmas.count(header["name"]) != 1) {
        this->lemmas[header["name"]];
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
    node_path.push_back(&this->lemmas[header["name"]]);
    try {
        ::split(header["node"].substr(1, header["node"].size() - 2), ",", [&node_path](const std::string &node_index) {
            if (node_index.size() == 0)
                return;
            int index = stoi(node_index);
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
        Logger::log(Logger::INFO,
                    header["name"] + header["node"] + " " + to_string(client.get_remote()) +
                    " clear");
        Node *node_remove = node_path.back();
        node_path.pop_back();
        if (node_path.size() > 0) {
            std::replace(node_path.back()->children.begin(),
                         node_path.back()->children.end(),
                         node_remove,
                         new Node);
            delete node_remove;
        } else {
            this->lemmas.erase(header["name"]);
        }
        return;
    }

    // push
    if (header.count("separator") == 1) {
        //std::list<Lemma *> *lemmas = &node_path.back()->lemmas;
        std::list<Lemma *> *lemmas_solver = &this->solvers[header["name"]][&client];

        uint32_t pushed = 0;
        uint32_t n = 0;
        int64_t push_rowid = -1;
        std::vector<net::Lemma> lemmas;
        std::istringstream is(payload);
        is >> lemmas;
        for (net::Lemma &netlemma:lemmas) {
            if (netlemma.smtlib.size() == 0)
                continue;
            // level is too deep, some more push involved somehow
            if ((uint32_t) netlemma.level >= node_path.size())
                continue;
            std::list<Lemma *> &node_lemmas = node_path[netlemma.level]->lemmas;
            auto it = std::find_if(node_lemmas.begin(), node_lemmas.end(), [&netlemma](const Lemma *other) {
                return other->smtlib == netlemma.smtlib;
            });
            if (it != node_lemmas.end()) {
                (*it)->increase();
                lemmas_solver->push_back(*it);
                if (this->db) {
                    SQLite3::Statement stmt = *this->db->prepare(
                            "UPDATE Lemma SET score=? WHERE smtlib=?;");
                    stmt.bind(1, (*it)->get_score());
                    stmt.bind(2, (*it)->smtlib);
                    stmt.exec();
                }
            } else {
                Lemma *lemma = new Lemma(netlemma.smtlib);
                pushed++;
                node_lemmas.push_back(lemma);
                lemmas_solver->push_back(node_lemmas.back());
                if (this->db) {
                    if (push_rowid < 0) {
                        SQLite3::Statement stmt = *this->db->prepare(
                                "INSERT INTO Push (ts,name,node,data) VALUES(?,?,?,?);");
                        stmt.bind(1, std::time(nullptr));
                        stmt.bind(2, header["name"]);
                        stmt.bind(3, header["node"]);
                        stmt.bind(4, to_string(header));
                        stmt.exec();
                        push_rowid = this->db->last_rowid();
                    }
                    SQLite3::Statement stmt = *this->db->prepare(
                            "INSERT INTO Lemma (pid,level,score,smtlib) VALUES(?,?,?,?);");
                    stmt.bind(1, push_rowid);
                    stmt.bind(2, netlemma.level);
                    stmt.bind(3, lemma->get_score());
                    stmt.bind(4, lemma->smtlib);
                    stmt.exec();
                }
            }
            n++;
        }

        Logger::log(Logger::INFO,
                    header["name"] + header["node"] + " " + to_string(client.get_remote()) +
                    " push [" + std::to_string(clauses_request) + "]\t" +
                    std::to_string(n) +
                    "\t(" + std::to_string(pushed) + "\tfresh, " + std::to_string(n - pushed) + "\tpresent)");

    } else { // pull
        std::list<Lemma *> lemmas;
        std::list<Lemma *> *lemmas_solver = &this->solvers[header["name"]][&client];

        for (auto node:node_path) {
            lemmas.merge(std::list<Lemma *>(node->lemmas));
        }
        lemmas.sort(Lemma::compare);

        header["separator"] = "\0";

        std::vector<net::Lemma> lemmas_send;
        uint32_t n = 0;
        for (auto lemma = lemmas.rbegin(); lemma != lemmas.rend(); ++lemma) {
            if (n >= clauses_request)
                break;

            auto it = std::find_if(lemmas_solver->begin(), lemmas_solver->end(), [&lemma](const Lemma *other) {
                return other->smtlib == (*lemma)->smtlib;
            });
            if (it != lemmas_solver->end())
                continue;

            lemmas_solver->push_back(*lemma);
            lemmas_send.push_back(net::Lemma((*lemma)->smtlib, 0));
            n++;
        }

        std::stringstream lemmas_dump;
        lemmas_dump << lemmas_send;

        header["lemmas"] = std::to_string(n);
        try {
            client.write(header, lemmas_dump.str());
        }
        catch (net::SocketException ex) {
            return;
        }

        if (n > 0)
            Logger::log(Logger::INFO,
                        header["name"] + header["node"] + " " + to_string(client.get_remote()) +
                        " pull [" + std::to_string(clauses_request) + "]\t" +
                        std::to_string(n));
    }
}


