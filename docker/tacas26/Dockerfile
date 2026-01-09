# Created by Tomas Kolarik
# Eternal glory to him

ARG TARGET=ubuntu:latest

FROM $TARGET
USER root

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt install -y apt-utils make cmake \
       build-essential nano bc wget zstd libgmp-dev bison flex git python3-dev parallel \
       gnuplot inkscape

ENV CMAKE_BUILD_TYPE=Release
ENV ENABLE_LINE_EDITING=OFF
ENV PARALLEL=ON

RUN mkdir tacas26

WORKDIR /tacas26

RUN \
    git config --global advice.detachedHead false && \
    git clone https://github.com/usi-verification-and-security/SMTS.git --branch tacas26-artifact --single-branch --depth 1

RUN git clone https://github.com/usi-verification-and-security/opensmt.git --branch smts-tacas26-artifact --single-branch --depth 1

WORKDIR /tacas26/SMTS
RUN \
    export CMAKE_BUILD_PARALLEL_LEVEL=$(nproc) && \
    sh bin/make_smts.sh

RUN rm -rf \
    graphviz \
    gui \
    build/lib \
    build/_deps/ptplib-src/tests \
    build/_deps/opensmt-src/regression* \
    build/_deps/opensmt-src/benchmark \
    build/_deps/opensmt-src/docs \
    build/_deps/opensmt-src/examples


WORKDIR /tacas26/opensmt
RUN \
    export CMAKE_BUILD_PARALLEL_LEVEL=$(nproc) && \
    make


WORKDIR /tacas26/SMTS/docker/tacas26
RUN \
    cp README.md LICENSE runme.sh /tacas26

WORKDIR /tacas26
RUN \
    bash SMTS/docker/tacas26/get_benchmarks.sh data SMTS/experiments/data/tacas26 && \
    rm -fr SMTS/experiments/data/tacas26 && \
    mkdir plots
