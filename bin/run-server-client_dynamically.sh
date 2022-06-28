#!/bin/bash
workdir=$(cd $(dirname $0); pwd)/../
echo ${workdir}
${workdir}/server/smts.py -p -pt 1 -l -el &

sleep 1
${workdir}/server/client.py 3000 ${workdir}/benchmarks/Ben-Amram-2010LMCS-Ex2.3_true-termination.c_Iteration1_Lasso_3-pieceTemplate.smt2.bz2

${workdir}/build/solver_opensmt -s127.0.0.1:3000 &
sleep 0.1
${workdir}/build/solver_opensmt -s127.0.0.1:3000 &
sleep 0.3
${workdir}/build/solver_opensmt -s127.0.0.1:3000 &
sleep 0.2
${workdir}/build/solver_opensmt -s127.0.0.1:3000 &
sleep 0.1
${workdir}/build/solver_opensmt -s127.0.0.1:3000 &
${workdir}/build/solver_opensmt -s127.0.0.1:3000 &
${workdir}/build/solver_opensmt -s127.0.0.1:3000 &
${workdir}/build/solver_opensmt -s127.0.0.1:3000 &
${workdir}/build/solver_opensmt -s127.0.0.1:3000 &
${workdir}/build/solver_opensmt -s127.0.0.1:3000 &
${workdir}/build/solver_opensmt -s127.0.0.1:3000 &


wait