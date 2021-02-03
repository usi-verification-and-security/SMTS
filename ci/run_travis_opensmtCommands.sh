#!/bin/bash

set -ev
cd ~
mkdir ${EXTERNALREPODIR}
cd ${EXTERNALREPODIR}
git clone https://github.com/usi-verification-and-security/opensmt.git --branch v2.0.1 --single-branch
cd opensmt
mkdir build 
cd build
cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${INSTALL} .. 
make -j8
make install