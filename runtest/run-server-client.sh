#!/bin/bash

OPENSMTDIR="build/_deps/opensmt-src"
./server/smts.py -d test.db -o4 -l  &
sleep 3
./server/client.py 3000  ${OPENSMTDIR}/regression/QF_UF/NEQ004_size4.smt2
sleep 3
./server/client.py 3000  ${OPENSMTDIR}/regression/QF_LIA/can_solve/ex10100_2600_100.smt2
sleep 3
./server/client.py 3000  ${OPENSMTDIR}/regression/QF_UF/php_3_3_40_sat.smt2
sleep 3
./server/client.py 3000  ${OPENSMTDIR}/regression/QF_UF/php_3_3_40_unsat.smt2
sleep 3
./server/client.py 3000  -t
sleep 1


