#!/bin/bash

source $(dirname "$0")/lib

if [[ -z $TARGET ]]; then
    ARGS=()
else
    ARGS=(--build-arg "TARGET=$TARGET")
fi

printf "Building Docker image %s located at %s ...\n" $IMAGE "$DIR"

exec docker build -t $IMAGE "${ARGS[@]}" "$DIR"
