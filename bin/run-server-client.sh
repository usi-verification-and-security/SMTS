#!/bin/bash

OPENSMTDIR=$(pwd)"/build/_deps/opensmt-src/regression"
./server/smts.py  -o4 -l  -fp ${OPENSMTDIR}/QF_UF/NEQ004_size4.smt2,${OPENSMTDIR}/QF_LIA/can_solve/ex10100_2600_100.smt2,${OPENSMTDIR}/QF_UF/php_3_3_40_sat.smt2,${OPENSMTDIR}/QF_UF/php_3_3_40_unsat.smt2
sleep 1


