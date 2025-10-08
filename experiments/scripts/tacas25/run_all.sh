#!/bin/bash

TACAS_SCRIPTS_DIR=$(dirname $(realpath "$0"))
SCRIPTS_DIR=$(realpath "$TACAS_SCRIPTS_DIR/../")
ROOT_DIR=$(realpath "$SCRIPTS_DIR/../../")

if [[ -z $OLD_ALG ]]; then
  VERSION=final-alg
  export nts=''
  os='1 2 4 8'
else
  VERSION=old-alg
  export nts=0
  os=8
fi

export TIMEOUT=1200

FILES=(
  "$ROOT_DIR/data/QF_LRA_files"
  "$ROOT_DIR/data/QF_LIA_files_1-1000"
  "$ROOT_DIR/data/QF_LIA_files_1001-2000"
  "$ROOT_DIR/data/QF_LIA_files_2001-3000"
  "$ROOT_DIR/data/QF_LIA_files_3001-4000"
  "$ROOT_DIR/data/QF_LIA_files_4001-6000"
  "$ROOT_DIR/data/QF_LIA_files_6001-8000"
  "$ROOT_DIR/data/QF_LIA_files_8001-10000"
  "$ROOT_DIR/data/QF_LIA_files_10001-12000"
  "$ROOT_DIR/data/QF_LIA_files_12001-13224"
)

for f in "${FILES[@]}"; do
  for o in $os; do
    if (( $o < 8 )); then
      ps=1
      ls=1
    else
      ps='1 0'
      ls='1 0'
    fi
    for p in $ps; do
      for l in $ls; do
        PARTITIONING=$p LEMMA_SHARING=$l os=$o "$TACAS_SCRIPTS_DIR/sbatch.sh" $VERSION "$f"
      done
    done
  done
done
