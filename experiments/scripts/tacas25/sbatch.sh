#!/bin/bash

export TACAS_SCRIPTS_DIR=$(dirname $(realpath "$0"))
export ROOT_DIR=$(realpath "$TACAS_SCRIPTS_DIR/../../../")

[[ -z $1 ]] && exec bash "$TACAS_SCRIPTS_DIR/runme.sh"

name=$(basename "$PWD")

time_opt=''
# time_opt='-t 48:00:00'

partition_opt='-p slim'
# partition_opt='-p fat'
# partition_opt=''

exclude_nodes='-x icsnode33,icsnode34,icsnode35,icsnode36,icsnode37,icsnode38'

# export TIMEOUT=300
# export TIMEOUT=1200

sbatch $time_opt $partition_opt $exclude_nodes -J "$name" -e slurm_e -o slurm_o "$TACAS_SCRIPTS_DIR/srunme.sh" "$@"
