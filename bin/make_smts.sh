#!/bin/bash

set -ev
if [ -d build ]; then rm -rf build; fi
mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
      -DCMAKE_CXX_FLAGS="${FLAGS}" \
      -DENABLE_LINE_EDITING:BOOL=${ENABLE_LINE_EDITING} \
      ..

[ -n $1 ] && NPROC=$1
[ -z $NPROC ] && NPROC=$(nproc)

make -j$NPROC
