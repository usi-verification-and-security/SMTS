#!/bin/bash

if [ $# != 1 ]; then
    echo "Usage: $0 <provide benchs dir>"
    exit 1
fi
n_benchmarks=$(find $1 -name '*.smt2.bz2' | wc -l)
echo "Benchmark set (total ${n_benchmarks}):"
../../server/smts.py -o8 -l &
sleep 2
  find $1 -name '*.smt2.bz2' |
  while read -r file;
  do
    echo $file
      ../../server/client.py  3000 $file
  done

