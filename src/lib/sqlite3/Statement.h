//
// Author: Matteo Marescotti
//

#ifndef SMTS_LIB_SQLITE3_STATEMENT_H
#define SMTS_LIB_SQLITE3_STATEMENT_H


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


#endif
