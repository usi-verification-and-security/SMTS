#!/bin/bash

hostfile="$1"
problem_path="$2"
totalWorkerNodes=$3

dir=$(dirname "$0")
root_dir=$(realpath "$dir/..")

#only works in linux
server_ip=$(/sbin/ip -o -4 addr list eth0 | awk '{print $4}' | cut -d/ -f1)
totalNumberOfSolverInstances=$(($totalWorkerNodes*4))

#run smts server
python3 "$root_dir/server/smts.py" -p &

sleep 1
#run lemma server
"$root_dir/build/lemma_server" -s$server_ip:3000 &

#run solver clients
# mpirun --mca btl_tcp_if_include eth0 --allow-run-as-root -np $totalNumberOfSolverInstances \
#   --hostfile "$hostfile" --use-hwthread-cpus --map-by node:pe=4 --bind-to none --report-bindings \
for ((i=1; i<=$totalNumberOfSolverInstances; ++i)); do
  "$root_dir/build/solver_opensmt" -s$server_ip:3000 &
done

#send instance
python3 "$root_dir/server/client.py" 3000 "$problem_path"

wait
