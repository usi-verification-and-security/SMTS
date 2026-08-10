#!/bin/bash

problem_path="$1"
totalWorkerNodes=$2

[[ -z $totalWorkerNodes ]] && totalWorkerNodes=$(nproc)

dir=$(dirname "$0")
root_dir=$(realpath "$dir/..")

#only works in linux
totalNumberOfSolverInstances=$(($totalWorkerNodes/2 + 1))

#run smts server
python3 "$root_dir/server/smts.py" -p -l -o $totalNumberOfSolverInstances -fp "$problem_path"
