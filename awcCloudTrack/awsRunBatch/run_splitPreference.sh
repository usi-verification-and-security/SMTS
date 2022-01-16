#!/bin/bash

if [ $# != 1 ]; then
    echo "Usage: $0 <provide benchs dir>"
    exit 1
fi

total=0

../../server/smts.py -o6 &
sleep 2
    for file in "$1"/*.smt2; do
#      echo  $file
      ../../server/client.py  3000 $file
      sleep 1
      ((total=total+1))
      if  [ ${total} == 10 ]
        then
#          echo "wait for result............." ${total}
        sleep 5
      fi
    done
#sleep 100
wait

