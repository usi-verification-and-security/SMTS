//
// Created by Matteo on 24/10/2016.
//

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <lib/net.h>
#include "fixedpoint.h"
#include "lib/lib.h"


z3::context context;

void push(Z3_fixedpoint_lemma_set s) {
    std::vector<std::string> lemmas;
    char *l;
    while ((l = (char *)Z3_fixedpoint_lemma_pop(context, s)) != nullptr) {
        //lemmas.push_back(l);
        std::cout << l << "\n";
        free(l);
    }
    //send them to server
}

void pull(Z3_fixedpoint_lemma_set s) {
    // ask for them to the server
    std::vector<std::string> lemmas;
    for (auto l:lemmas) {
        Z3_fixedpoint_lemma_push(context, s, l.c_str());
    }
}

int main(int argc, char **argv) {

    std::vector<net::Lemma> ll;
    ll.push_back(net::Lemma("asd",0));
    ll.push_back(net::Lemma("dsa",1));

    std::thread t([&]{
        net::Socket s((uint16_t) 4000);
        std::shared_ptr<net::Socket> c=s.accept();
        net::Header header;
        std::string payload;
        while(1){
            c->read(header, payload);
            std::istringstream is(payload);
            std::vector<net::Lemma> rl;
            is >> rl;
            std::cout << rl.size() << "\n";
        }
    });
    usleep(10000);

    std::stringstream s;
    s << ll;
    net::Header header;
    std::string payload;
    net::Socket c(net::Address("127.0.0.1",4000));
    c.write(header,s.str());
    //std::cout << ll;
//    std::vector<net::Lemma> cc;
//    std::istringstream is(s.str());
//    is >> cc;
//    for(auto i:cc){
//        std::cout << i.level << ":"<<i.smtlib<<"\n";
//    }


    exit(0);
    z3::fixedpoint f(context);
    z3::params p(context);
    p.set(":engine", context.str_symbol("spacer"));
    f.set(p);

    z3::solver solver(context);

    if (argc > 1) {
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            exit(-1);
        }

        std::string content;
        file.seekg(0, std::ios::end);
        content.resize((unsigned long) file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(&content[0], content.size());
        file.close();

        Z3_ast_vector v = Z3_fixedpoint_from_string(context, f, content.c_str());
        Z3_ast a = Z3_ast_vector_get(context, v, 0);

        Z3_fixedpoint_set_lemma_pull_callback(context, f, pull);
        Z3_fixedpoint_set_lemma_push_callback(context, f, push);

        Z3_lbool res = Z3_fixedpoint_query(context, f, a);
        if (res == Z3_L_TRUE) std::cout << "VERIFICATION FAILED\n";
        else if (res == Z3_L_FALSE) std::cout << "VERIFICATION SUCCESSFUL\n";

    }
}