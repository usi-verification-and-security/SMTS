//
// Author: Matteo Marescotti
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
//#include "../../build/_deps/partition-channel-src/PartitionChannelLibrary.h"

LemmaServer::LemmaServer(uint16_t port, const std::string &server, const std::string &db_filename, bool send_again) :
        Server(port),
        send_again(send_again) {
    if (server.size()) {
        this->server.reset(new net::Socket(server));
        net::Header header;
        header["lemmas"] = ":" + std::to_string(port);
        this->server->write(header, "");
        this->add_socket(this->server);
    }

#ifdef SQLITE_IS_ON
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
#endif
};

void LemmaServer::handle_accept(net::Socket &client) {
    Logger::log(Logger::INFO, "+ " + to_string(client.get_remote()));
}

void LemmaServer::handle_close(net::Socket &client) {
    Logger::log(Logger::INFO, "- " + to_string(client.get_remote()));
//    if(pool.get_tasks_running()>0)
    {
        //pool.paused= false;
//        if(pool.get_tasks_total()>0)
//            pool.reset();
        if (&client == this->server.get())
        {
            Logger::log(Logger::INFO, "server connection closed. ");
            this->stop();
            return;
        }
//        solvers_mutex.lock();
        for (auto &pair:this->solvers) {
            if (this->solvers[pair.first].count(&client)) {
                this->solvers[pair.first].erase(&client);
            }
        }
//        solvers_mutex.unlock();
    }
}

void LemmaServer::handle_exception(net::Socket &client, const std::exception &ex) {
    Logger::log(Logger::WARNING, "Exception from: " + to_string(client.get_remote()) + ": " + ex.what());
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

#ifdef ENABLE_DEBUGING
    std::cout << "[LemmServer] Start Only LemmaOperation for node -> "+header["node"]<< std::endl;
#endif
    uint32_t clauses_request = 0;
    std::cout<<header["lemmas"]<<endl;
    if (header["lemmas"] != "0")
        clauses_request = (uint32_t) stoi(header["lemmas"].substr(1));

    std::vector<Node *> node_path;
    node_path.push_back(&this->lemmas[header["name"]]);
//    solvers_mutex.unlock();
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
//            solvers_mutex.lock();
            this->lemmas.erase(header["name"]);
//            solvers_mutex.unlock();
        }
        return;
    }

    bool push;
    if (header["lemmas"][0] == '+')
        push = true;
    else if (header["lemmas"][0] == '-')
        push = false;
    else
        return;

    // push
    if (push) {
        //std::list<Lemma *> *lemmas = &node_path.back()->lemmas;
//        solvers_mutex.lock();
        std::map<Lemma *, bool> &lemmas_solver = this->solvers[header["name"]][&client];
//        solvers_mutex.unlock();
        uint32_t pushed = 0;
        uint32_t n = 0;
        int64_t push_rowid = -1;
        std::vector<net::Lemma> lemmas;
        std::istringstream is(payload);
        is >> lemmas;
        for (net::Lemma &lemma:lemmas) {
            if (lemma.smtlib.size() == 0)
                continue;
            // level is too deep, some more push involved somehow
            if ((uint32_t) lemma.level >= node_path.size())
                continue;
            for (int level = 0; level <= lemma.level; level++) {
                Lemma *l = node_path[level]->get(lemma);
                if (l) {
                    l->increase();
                    if (!lemmas_solver[l]) {
                        lemmas_solver[l] = true;
#ifdef SQLITE_IS_ON
                        if (this->db) {
                            SQLite3::Statement stmt = *this->db->prepare(
                                    "UPDATE Lemma SET score=? WHERE smtlib=?;");
                            stmt.bind(1, l->get_score());
                            stmt.bind(2, l->smtlib);
                            stmt.exec();
                        }
#endif
                    }
                    break;
                } else if (level == lemma.level) {
                    pushed++;
                    l = node_path[level]->add_lemma(lemma);
                    lemmas_solver[l] = true;
#ifdef SQLITE_IS_ON
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
                        stmt.bind(2, lemma.level);
                        stmt.bind(3, l->get_score());
                        stmt.bind(4, l->smtlib);
                        stmt.exec();
                        l->id = (uint32_t) this->db->last_rowid();
                    }
#endif
                }
            }
            n++;
        }
#ifdef ENABLE_DEBUGING
        std::cout << "[LemmServer] EPush_OnlyLemmaOperation for node -> "+header["node"]<< std::endl;
        Logger::log(Logger::PUSH,
                    header["name"] + header["node"] + " " + to_string(client.get_remote()) +
                    " push [" + std::to_string(clauses_request) + "]\t" +
                    std::to_string(n) +
                    "\t(" + std::to_string(pushed) + "\tfresh, " + std::to_string(n - pushed) + "\tpresent)");
#endif

    } else { // pull
//        std::cout << "[LemmServer] SPull_OnlyLemmaSelection for node -> "+header["node"]<< std::endl;
        std::list<Lemma *> lemmas;
//        solvers_mutex.lock();
        std::map<Lemma *, bool> &lemmas_solver = this->solvers[header["name"]][&client];

        for (auto node:node_path) {
            node->fill(lemmas);
        }
        lemmas.sort(Lemma::compare);

        std::vector<net::Lemma> lemmas_send;
        uint32_t n = 0;
        for (auto lemma = lemmas.rbegin(); lemma != lemmas.rend(); ++lemma) {
            if (n >= clauses_request)
                break;

            if (lemmas_solver[*lemma])
                continue;

            if (!this->send_again)
                lemmas_solver[*lemma] = true;
            lemmas_send.push_back(net::Lemma((*lemma)->smtlib, 0));
            n++;
        }
        header["lemmas"] = std::to_string(n);
#ifdef ENABLE_DEBUGING
        std::cout << "[LemmServer] EPull_OnlyLemmaSelection for node -> "+header["node"]<< std::endl;
#endif

        client.write(header, ::to_string(lemmas_send));

#ifdef ENABLE_DEBUGING
        std::cout << "[LemmServer] EWriting lemmas to node -> "+header["node"]<< std::endl;
        if (n > 0)
            Logger::log(Logger::PULL,
                        header["name"] + header["node"] + " " + to_string(client.get_remote()) +
                        " pull [" + std::to_string(clauses_request) + "]\t" +
                        std::to_string(n));
#endif

    }
}


