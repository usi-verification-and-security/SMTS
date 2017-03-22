//
// Created by Matteo on 02/12/2016.
//

#ifndef CLAUSE_SERVER_STATEMENT_H
#define CLAUSE_SERVER_STATEMENT_H


namespace SQLite3 {
    class Statement {
    private:
        const void *stmt;
    public:
        Statement(void *);

        ~Statement();

        void bind(int) const;

        void bind(int, int64_t) const;

        void bind(int, const std::string &) const;

        void bind(int, const char *, int len = -1) const;

        void reset() const;

        void clear() const;

        void exec() const;

    };
}


#endif //CLAUSE_SERVER_STATEMENT_H
