#!/bin/bash

if [ $# != 2 ]; then
    echo "Usage: $0 <provide benchs dir>"
    exit 1
fi

#total=0

#../../server/smts.py -o6 &
#sleep 2
    for file in "$1"/*.bz2; do
      echo "'$file'"
      ../../server/client.py  3000 $file

    done
#wait
#n_node=0
#n_benchmarks=$(ls ${1}/*.bz2 |wc -l)
#echo "Benchmark set (total ${n_benchmarks}):"
#((n_node=((n_benchmarks/($2*3)))))
#echo "Number of Nodes (total ${n_node}):"
#find "$1" -name '*.bz2' |
#while read -r file;
#  do
#((n_node=n_node-1))
#echo $n_node
#if  [ ${n_node} == 0 ]
#        then
#          echo 'hi'
#          break
#fi
#done
