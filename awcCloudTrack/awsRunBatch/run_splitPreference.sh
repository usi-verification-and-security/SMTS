#!/bin/bash

if [ $# != 1 ]; then
    echo "Usage: $0 <provide benchs dir>"
    exit 1
fi

total=0

../../server/smts.py -o10 &
sleep 2
    for file in "$1"/*.smt2; do
      sleep 1
      ../../server/client.py  3000 $file
      ((total=total+1))
      if  [ ${total} == 10 ]
        then
          sleep 5
          total=0
      fi
    done
wait

