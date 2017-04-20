//
// Created by Matteo on 25/10/2016.
//

#ifndef CLAUSE_SERVER_FIXEDPOINT_H
#define CLAUSE_SERVER_FIXEDPOINT_H

#include "z3++.h"


namespace z3 {
    class fixedpoint : public object {
        Z3_fixedpoint m_fixedpoint;

        void init(Z3_fixedpoint f) {
            m_fixedpoint = f;
            Z3_fixedpoint_inc_ref(ctx(), f);
        }

    public:
        fixedpoint(context &c) : object(c) { init(Z3_mk_fixedpoint(c)); }

        fixedpoint(context &c, Z3_fixedpoint s) : object(c) { init(s); }

        fixedpoint(fixedpoint const &s) : object(s) { init(s.m_fixedpoint); }

        ~fixedpoint() { Z3_fixedpoint_dec_ref(ctx(), m_fixedpoint); }

        operator Z3_fixedpoint() const { return m_fixedpoint; }

        fixedpoint &operator=(fixedpoint const &s) {
            Z3_fixedpoint_inc_ref(s.ctx(), s.m_fixedpoint);
            Z3_fixedpoint_dec_ref(ctx(), m_fixedpoint);
            m_ctx = s.m_ctx;
            m_fixedpoint = s.m_fixedpoint;
            return *this;
        }

        void set(params const &p) {
            Z3_fixedpoint_set_params(ctx(), m_fixedpoint, p);
            check_error();
        }
    };
}

#endif //CLAUSE_SERVER_FIXEDPOINT_H
