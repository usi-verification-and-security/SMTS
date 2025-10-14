#!/bin/bash

TACAS_SCRIPTS_DIR=$(dirname $(realpath "$0"))
SCRIPTS_DIR=$(realpath "$TACAS_SCRIPTS_DIR/../")
ROOT_DIR=$(realpath "$SCRIPTS_DIR/../../")
LOCAL_DATA_DIR=$(realpath "$ROOT_DIR/local")

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

## Due to limited job time limit (e.g. 24 h), we need to split the task into subtasks
CHUNK_SIZE=100

FILES=(
  "$LOCAL_DATA_DIR/QF_LRA_files"
  "$LOCAL_DATA_DIR/QF_LIA_files"
)

for f in "${FILES[@]}"; do
  [[ -r $f ]] || {
    printf 'File "%s" is not readable.\n' "$f" >&2
    exit 2
  }

  wcl=$(wc -l <"$f")
  for (( lo = 1; lo <= $wcl; lo += $CHUNK_SIZE )); do
    hi=$(( lo + $CHUNK_SIZE - 1 ))
    (( hi > $wcl )) && hi=$wcl
    if (( lo == 1 && hi == $wcl )); then
      lo_arg=
      hi_arg=
    else
      lo_arg=$lo
      hi_arg=$hi
    fi

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
          PARTITIONING=$p LEMMA_SHARING=$l os=$o "$TACAS_SCRIPTS_DIR/sbatch.sh" $VERSION "$f" $lo_arg $hi_arg
        done
      done
    done
  done
done
