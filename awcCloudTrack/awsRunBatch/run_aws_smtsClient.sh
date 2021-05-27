#!/bin/bash

if [ $# != 1 ]; then
    echo "Usage: $0 <provide shared bucket dir>"
    exit 1
fi

for folder in "$1"/*; do
    for file in "$folder"/*/*; do
      SMTS/server/client.py 3000 $file
    done
done



