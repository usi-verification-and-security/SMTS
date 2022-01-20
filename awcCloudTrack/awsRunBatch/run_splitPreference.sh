#!/bin/bash

if [ $# != 1 ]; then
    echo "Usage: $0 <provide benchs dir>"
    exit 1
fi

total=0

#../../server/smts.py -o6 &
#sleep 2
    for file in "$1"/*.bz2; do
      echo "'$file'"
      sleep 1
      ../../server/client.py  3000 $file

    done
wait
#total=0
#find "/home/masoud/benchmarks/QF_UF/" -name '*smt2.bz2' |
#while read -r file;
#  do
#((total=total+1))
#if  [ ${total} == 20 ]
#        then
#          ((total=total-1))
#          rm $file
#    fi
#  done
