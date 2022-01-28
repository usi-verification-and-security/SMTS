#!/bin/bash

set -ev
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_FLAGS="$FLAGS" -DDEBUG_SMTS=true ..
make -j4