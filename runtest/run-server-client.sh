#!/bin/bash

./server/smts.py -d test.db -o4 -l  &
sleep 4
./server/client.py 3000 /home/circleci/externalRepo/opensmt/regression_splitting/instances/init_unsat.smt2;
./server/client.py 3000 /home/circleci/externalRepo/opensmt/regression_splitting/instances/init_unsat-deep.smt2;
./server/client.py 3000 /home/circleci/externalRepo/opensmt/regression_splitting/instances/p2-zenonumeric_s6.smt2;
./server/client.py 3000 /home/circleci/externalRepo/opensmt/regression_splitting/instances/tta_startup_simple_startup_3nodes.synchro.base-deep.smt2;
./server/client.py 3000  -t
sleep 3


