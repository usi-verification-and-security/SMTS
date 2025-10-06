#!/bin/sh

#SBATCH --nodes=1
#SBATCH --tasks-per-node=1
#SBATCH --cpus-per-task=20
#SBATCH --time=2-00:00:00

srun bash "$SCRIPTS_DIR/runme.sh" "$@"
