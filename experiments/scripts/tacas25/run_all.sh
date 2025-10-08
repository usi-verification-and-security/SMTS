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
  "$ROOT_DIR/data/QF_LRA_files_1-250"
  "$ROOT_DIR/data/QF_LRA_files_251-500"
  "$ROOT_DIR/data/QF_LRA_files_501-750"
  "$ROOT_DIR/data/QF_LRA_files_751-1000"
  "$ROOT_DIR/data/QF_LRA_files_1001-1250"
  "$ROOT_DIR/data/QF_LRA_files_1251-1500"
  "$ROOT_DIR/data/QF_LRA_files_1501-1753"
  "$ROOT_DIR/data/QF_LIA_files_1-250"
  "$ROOT_DIR/data/QF_LIA_files_251-500"
  "$ROOT_DIR/data/QF_LIA_files_501-750"
  "$ROOT_DIR/data/QF_LIA_files_751-1000"
  "$ROOT_DIR/data/QF_LIA_files_1001-1250"
  "$ROOT_DIR/data/QF_LIA_files_1251-1500"
  "$ROOT_DIR/data/QF_LIA_files_1501-1750"
  "$ROOT_DIR/data/QF_LIA_files_1751-2000"
  "$ROOT_DIR/data/QF_LIA_files_2001-2250"
  "$ROOT_DIR/data/QF_LIA_files_2251-2500"
  "$ROOT_DIR/data/QF_LIA_files_2501-2750"
  "$ROOT_DIR/data/QF_LIA_files_2751-3000"
  "$ROOT_DIR/data/QF_LIA_files_3001-3250"
  "$ROOT_DIR/data/QF_LIA_files_3251-3500"
  "$ROOT_DIR/data/QF_LIA_files_3501-3750"
  "$ROOT_DIR/data/QF_LIA_files_3751-4000"
  "$ROOT_DIR/data/QF_LIA_files_4001-4250"
  "$ROOT_DIR/data/QF_LIA_files_4251-4500"
  "$ROOT_DIR/data/QF_LIA_files_4501-4750"
  "$ROOT_DIR/data/QF_LIA_files_4751-5000"
  "$ROOT_DIR/data/QF_LIA_files_5001-5250"
  "$ROOT_DIR/data/QF_LIA_files_5251-5500"
  "$ROOT_DIR/data/QF_LIA_files_5501-5750"
  "$ROOT_DIR/data/QF_LIA_files_5751-6000"
  "$ROOT_DIR/data/QF_LIA_files_6001-6250"
  "$ROOT_DIR/data/QF_LIA_files_6251-6500"
  "$ROOT_DIR/data/QF_LIA_files_6501-6750"
  "$ROOT_DIR/data/QF_LIA_files_6751-7000"
  "$ROOT_DIR/data/QF_LIA_files_7001-7250"
  "$ROOT_DIR/data/QF_LIA_files_7251-7500"
  "$ROOT_DIR/data/QF_LIA_files_7501-7750"
  "$ROOT_DIR/data/QF_LIA_files_7751-8000"
  "$ROOT_DIR/data/QF_LIA_files_8001-8250"
  "$ROOT_DIR/data/QF_LIA_files_8251-8500"
  "$ROOT_DIR/data/QF_LIA_files_8501-8750"
  "$ROOT_DIR/data/QF_LIA_files_8751-9000"
  "$ROOT_DIR/data/QF_LIA_files_9001-9250"
  "$ROOT_DIR/data/QF_LIA_files_9251-9500"
  "$ROOT_DIR/data/QF_LIA_files_9501-9750"
  "$ROOT_DIR/data/QF_LIA_files_9751-10000"
  "$ROOT_DIR/data/QF_LIA_files_10001-10250"
  "$ROOT_DIR/data/QF_LIA_files_10251-10500"
  "$ROOT_DIR/data/QF_LIA_files_10501-10750"
  "$ROOT_DIR/data/QF_LIA_files_10751-11000"
  "$ROOT_DIR/data/QF_LIA_files_11001-11250"
  "$ROOT_DIR/data/QF_LIA_files_11251-11500"
  "$ROOT_DIR/data/QF_LIA_files_11501-11750"
  "$ROOT_DIR/data/QF_LIA_files_11751-12000"
  "$ROOT_DIR/data/QF_LIA_files_12001-12250"
  "$ROOT_DIR/data/QF_LIA_files_12251-12500"
  "$ROOT_DIR/data/QF_LIA_files_12501-12750"
  "$ROOT_DIR/data/QF_LIA_files_12751-13000"
  "$ROOT_DIR/data/QF_LIA_files_13001-13224"
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
