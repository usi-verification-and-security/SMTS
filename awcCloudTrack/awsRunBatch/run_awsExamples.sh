#!/bin/bash

if [ $# != 2 ]; then
    echo "Usage: $0 <provide path to the execution and bench's paths>"
    exit 1
fi

while IFS="" read -r p || [ -n "$p" ]
do
  printf '%s ' "$1" "python3 --file $p --Nworker 2"
  "$1" "python3 $p --Nworker 2"
  wait
done < "$2"

