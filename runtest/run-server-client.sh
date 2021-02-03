#!/bin/bash

./server/smts.py -d test.db -o4 -l  &
sleep 12
./server/client.py 3000 /home/circleci/externalRepo/opensmt/regression_splitting/instances/init_unsat.smt2 
sleep 10


