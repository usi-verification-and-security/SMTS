#!/bin/bash

hostfile="$1"
problem_path="$2"
totalWorkerNodes=$3

dir=$(dirname "$0")
root_dir=$(realpath "$dir/..")

#only works in linux
server_ip=$(/sbin/ip -o -4 addr list eth0 | awk '{print $4}' | cut -d/ -f1)
# Workers use the same instance type as the leader, so the leader's CPU count is theirs.
solversPerNode=$(( $(nproc) / 2 ))
(( $solversPerNode < 1 )) && solversPerNode=1
totalNumberOfSolverInstances=$(($totalWorkerNodes*$solversPerNode))

#run smts server
python3 "$root_dir/server/smts.py" -p &
server_pid=$!

sleep 1
#run lemma server
"$root_dir/build/lemma_server" -s$server_ip:3000 &

# Wait until the SMTS server is accepting connections: each client connects once and exits
# on failure, so launching hundreds of remote clients before the server listens loses them.
for _ in $(seq 1 60); do
  ss -ltn | grep -q ':3000 ' && break
  sleep 0.5
done

#run solver clients
# orte_abort_on_non_zero_status=0: one client exiting must not make mpirun kill all others.
# one client per physical core: pe=2 (2 hyperthreads = 1 core on m6i) and clients per node = vCPUs/2
mpirun --mca btl_tcp_if_include eth0 --mca orte_abort_on_non_zero_status 0 --allow-run-as-root -np $totalNumberOfSolverInstances \
  --hostfile "$hostfile" --use-hwthread-cpus --map-by node:pe=2 --bind-to none --report-bindings \
  "$root_dir/build/solver_opensmt" -s$server_ip:3000 &

#send instance (client.py returns right after submitting; the server prints the answer)
python3 "$root_dir/server/client.py" 3000 "$problem_path"

# The server exits once the instance is solved. Wait for it, then stop the lemma server and
# mpirun (and with it all remote clients) instead of a bare `wait`: some clients keep running
# after an answer, which held the job until the 200s kill and made the harness record solved
# instances as TIMEOUT.
wait $server_pid
kill $(jobs -p) 2>/dev/null
sleep 2
kill -9 $(jobs -p) 2>/dev/null
exit 0
