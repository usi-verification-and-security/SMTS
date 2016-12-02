//
// Created by Matteo on 02/12/2016.
//

#ifndef CLAUSE_SERVER_STATEMENT_H
#define CLAUSE_SERVER_STATEMENT_H


namespace SQLite3 {
    class Statement {
    private:
        void *stmt;
    public:
        Statement(void *);

        ~Statement();

        void bind(int);

        void bind(int, int);

        void bind(int, const char *, int);

        void bind(int, char *, int, void(*)(void *));

        void reset();

        void clear();

        void exec();

    };
}


#endif //CLAUSE_SERVER_STATEMENT_H
