#!/bin/bash

source $(dirname "$0")/lib

# ARCHIVE=${REPO}.tar.gz
ARCHIVE=${REPO}.tar

printf "Saving Docker image %s to %s ...\n" $IMAGE "$ARCHIVE"

# exec docker save $IMAGE | gzip >"$ARCHIVE"
exec docker save $IMAGE >"$ARCHIVE"
