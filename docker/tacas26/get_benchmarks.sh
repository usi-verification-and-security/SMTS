#!/bin/bash

DIR=$(realpath "$1")
BENCH_DIR="$DIR/benchmarks"

EXPERIMENT_DATA_DIR=$(realpath "$2")

set -e

if [[ -e $DIR ]]; then
    printf 'I will wipe out the provided directory %s ... [y] ' "$DIR"
    read a
    [[ $a == y ]] || exit 1
    rm -fr "$DIR"
fi

mkdir -p "$BENCH_DIR"

cd "$BENCH_DIR"

TRACKS=(non-incremental)
## 2025
TRACK_RECORDS=(15493090)
LOGICS=(QF_LIA QF_LRA)
for itrack in ${!TRACKS[@]}; do
    track=${TRACKS[$itrack]}
    rec=${TRACK_RECORDS[$itrack]}
    for log in ${LOGICS[@]}; do
        file=${log}.tar.zst
        dst_file=${track}_zst/$file
        mkdir -p ${track} ${track}_zst
        wget https://zenodo.org/records/$rec/files/$file -O $dst_file
        printf "Unpacking %s -> %s ...\n" $dst_file $track
        tar xf $dst_file --overwrite
    done
done

cd "$DIR"

for log in ${LOGICS[@]}; do
    find "$BENCH_DIR" -path '**/QF_LIA/**' -type f -name '*.smt2' >${log}_files

    cp "$EXPERIMENT_DATA_DIR"/outputs/opensmt/$log/opensmt_${log}_files .
    sed -i "s|/home/kolart/smt-comp/benchmarks/storage/2025|$BENCH_DIR|" opensmt_${log}_files
    cat opensmt_${log}_files | awk '{ if ($3 > 1) { print $1 } }' >${log}_files_gt1s
    shuf opensmt_${log}_files | awk '{ if ($3 > 1) { files[int($3)]=$1 } } END { for (t in files) { print files[t]} }' | shuf >${log}_files_uniq
    rm opensmt_${log}_files
done
