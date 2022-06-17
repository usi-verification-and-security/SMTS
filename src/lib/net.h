/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMTS_LIB_NET_H
#define SMTS_LIB_NET_H

#include <PTPLib/net/Header.hpp>
#include <PTPLib/net/Lemma.hpp>

#include "net/Address.h"
#include "net/Server.h"
#include "net/Socket.h"

namespace PTPLib::common {
    template<typename T, typename F>
    class capture_move {
        T x;
        F f;
    public:
        capture_move(T && x, F && f)
                : x{std::forward<T>(x)}, f{std::forward<F>(f)} {}

        template<typename ...Ts>
        auto operator()(Ts && ...args) -> decltype(f(x, std::forward<Ts>(args)...)) {
            return f(x, std::forward<Ts>(args)...);
        }

        template<typename ...Ts>
        auto operator()(Ts && ...args) const -> decltype(f(x, std::forward<Ts>(args)...)) {
            return f(x, std::forward<Ts>(args)...);
        }
    };

    template<typename T, typename F>
    capture_move<T, F> capture(T && x, F && f) {
        return capture_move<T, F>(std::forward<T>(x), std::forward<F>(f));
    }
}

#endif
