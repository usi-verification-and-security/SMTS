#!/bin/bash

DIR=$(realpath "$1")

set -e

if [[ -e $DIR ]]; then
    printf 'I will wipe out the provided directory %s ... [y] ' "$DIR"
    read a
    [[ $a == y ]] || exit 1
    rm -fr "$DIR"
fi

mkdir -p "$DIR"

cd "$DIR"

git clone https://github.com/usi-verification-and-security/SMTS.git --single-branch --branch tacas26-artifact-branch --depth 1
cd SMTS
sh bin/make_smts.sh
rm -rf \
    graphviz \
    gui \
    build/lib \
    build/_deps/ptplib-src/tests \
    build/_deps/opensmt-src/regression* \
    build/_deps/opensmt-src/benchmark \
    build/_deps/opensmt-src/docs \
    build/_deps/opensmt-src/examples \

cd "$DIR"
git clone https://github.com/usi-verification-and-security/opensmt.git --single-branch --branch smts-tacas26-artifact --depth 1
cd opensmt
make

cd "$DIR"
cp SMTS/docker/tacas26/runme.sh .
bash SMTS/docker/tacas26/get_benchmarks.sh data SMTS/experiments/data/tacas26
rm -fr SMTS/experiments/data/tacas26
mkdir plots
