#!/bin/bash
/usr/sbin/sshd &

PATH="$PATH:/opt/openmpi/bin/"
BASENAME="${0##*/}"
log () {
  echo "${BASENAME} - ${1}"
}
HOST_FILE_PATH="/tmp/hostfile"
#aws s3 cp $S3_INPUT $SCRATCH_DIR
#tar -xvf $SCRATCH_DIR/*.tar.gz -C $SCRATCH_DIR

sleep 1
echo main node: ${AWS_BATCH_JOB_MAIN_NODE_INDEX}
echo this node: ${AWS_BATCH_JOB_NODE_INDEX}

#********* Set child by default, switch to main if on node index 0, switch to lemma if on node index 1 *****************
NODE_TYPE="child"

if [ "${AWS_BATCH_JOB_MAIN_NODE_INDEX}" == "${AWS_BATCH_JOB_NODE_INDEX}" ]; then
  log "Running synchronize as the main node"
  NODE_TYPE="main"
fi


if [ "$((AWS_BATCH_JOB_MAIN_NODE_INDEX + 1))" == "${AWS_BATCH_JOB_NODE_INDEX}" ];
  then
    if  [ "${LemmaServer}" == "ON" ]
      then
        if  [ "${LemmaServer_onServerNode}" == "N" ]
          then
            log "Running synchronize as the LemmaServer node"
            NODE_TYPE="lemma"
          fi
      fi
fi
#**************************************************** SMTS parallelization node ****************************************
wait_for_nodes () {

  log "Running as main node"
  touch $HOST_FILE_PATH
  ip=$(/sbin/ip -o -4 addr list eth0 | awk '{print $4}' | cut -d/ -f1)

  availablecores=$(nproc)
  log "main details (ip:cores) -> $ip:$availablecores"
  log "main IP: $ip"
  echo "$ip" >> $HOST_FILE_PATH
  if  [ "${DownloadFromS3}" == "N" ]
      then
      git clone https://github.com/usi-verification-and-security/SMT-LIB-benchmarks.git --single-branch
      echo Distributing problem from DockerImage: SMTS/${COMP_S3_PROBLEM_PATH}
    else
      echo Downloading problem from S3: ${COMP_S3_PROBLEM_PATH}
        if [[ "${COMP_S3_PROBLEM_PATH}" == *".xz" ]];
          then
            aws s3 cp s3://${S3_BKT}/${COMP_S3_PROBLEM_PATH} test.smt2.xz
            unxz test.smt2.xz
          else
            aws s3 cp s3://${S3_BKT}/${COMP_S3_PROBLEM_PATH} test.smt2
        fi
  fi
  if  [ "${LemmaServer}" == "ON" ]
      then
      if  [ "${LemmaServer_onServerNode}" == "Y" ]
        then
          python3 SMTS/server/smts.py -l &
      fi

   else
       python3 SMTS/server/smts.py  &
  fi
  sleep 1
  #echo "$ip" >> $HOST_FILE_PATH
  lines=$(ls -dq /tmp/hostfile* | wc -l)

  while [ "${AWS_BATCH_JOB_NUM_NODES}" -gt "${lines}" ]
    do
      cat $HOST_FILE_PATH
      lines=$(ls -dq /tmp/hostfile* | wc -l)
      log "$lines out of $AWS_BATCH_JOB_NUM_NODES nodes joined, check again in 0.5 second"
      sleep 0.5
    done

  python3 SMTS/bin/make_combined_hostfile.py ${ip}
#  cat SMTS/bin/combined_hostfile
  #IFS=$'\n' read -d '' -r -a workerNodes < SMTS/bin/combined_hostfile
  #for worker_ip in "${workerNodes[@]}"
  #do
  #  read -ra node_ip <<<${worker_ip}
  #  echo "$ip" >>  SMTS/bin/"${node_ip[0]}"
  #  if  [ "${node_ip[0]}" == "$ip" ]
  #  then
  #    echo "SMTS Server is running..."
  #    mpirun --mca btl_tcp_if_include eth0 --allow-run-as-root -np 1 --hostfile SMTS/bin/"${node_ip[0]}" python3 SMTS/server/smts.py  -l  &
  #    sleep 2
  #  else
   #   mpirun --mca btl_tcp_if_include eth0 --allow-run-as-root -np 1 --hostfile SMTS/bin/"${node_ip[0]}" SMTS/build/solver_opensmt -s ${node_ip[0]}:3000 &
   #   sleep 1
   # fi
  #done

  if  [ "${SendInChunk}" == "Y" ]
   then
     #************** SMTS could build a tree of instances and send one by one to the solvers****************************
       find SMTS/${COMP_S3_PROBLEM_PATH} -name '*.smt2.bz2' |
        while read -r file;
        do
          echo $file
          SMTS/server/client.py 3000 $file
          sleep 1
        done
  else
    if  [ "${DownloadFromS3}" == "N" ]
    then
      SMTS/server/client.py 3000 SMTS/${COMP_S3_PROBLEM_PATH}
    else
      SMTS/server/client.py 3000 test.smt2
    fi
  fi
  wait
}

#**************************************************** OpenSMT Solver node **********************************************
report_to_master () {
  # extract ip
  ip=$(/sbin/ip -o -4 addr list eth0 | awk '{print $4}' | cut -d/ -f1)
  availablecores=$(nproc)

  log "solver node${AWS_BATCH_JOB_NODE_INDEX} -> $ip:$availablecores, reporting to the server node -> ${AWS_BATCH_JOB_MAIN_NODE_PRIVATE_IPV4_ADDRESS}"

  echo "$ip" >> $HOST_FILE_PATH${AWS_BATCH_JOB_NODE_INDEX}
  ping -c 3 ${AWS_BATCH_JOB_MAIN_NODE_PRIVATE_IPV4_ADDRESS}
  until scp $HOST_FILE_PATH${AWS_BATCH_JOB_NODE_INDEX} ${AWS_BATCH_JOB_MAIN_NODE_PRIVATE_IPV4_ADDRESS}:$HOST_FILE_PATH${AWS_BATCH_JOB_NODE_INDEX}
  do
    echo "Sleeping 1 seconds and trying again"
  done
  #echo "$ip slots=2" >> $HOST_FILE_PATH${AWS_BATCH_JOB_NODE_INDEX}
  mpirun --mca btl_tcp_if_include eth0 --allow-run-as-root -np ${NSolver}  --hostfile $HOST_FILE_PATH${AWS_BATCH_JOB_NODE_INDEX} sh -c "SMTS/build/solver_opensmt -s ${AWS_BATCH_JOB_MAIN_NODE_PRIVATE_IPV4_ADDRESS}:3000 2>&1" &

  ps -ef | grep sshd
  wait
}

#**************************************************** Lemmas Server node **********************************************
startLemmaServer () {
  # extract ip
  ip=$(/sbin/ip -o -4 addr list eth0 | awk '{print $4}' | cut -d/ -f1)
  availablecores=$(nproc)

  log "LemmaServer node${AWS_BATCH_JOB_NODE_INDEX} -> $ip:$availablecores, Connect to the server node -> ${AWS_BATCH_JOB_MAIN_NODE_PRIVATE_IPV4_ADDRESS}"

  echo "$ip" >> $HOST_FILE_PATH${AWS_BATCH_JOB_NODE_INDEX}
  ping -c 3 ${AWS_BATCH_JOB_MAIN_NODE_PRIVATE_IPV4_ADDRESS}
  until scp $HOST_FILE_PATH${AWS_BATCH_JOB_NODE_INDEX} ${AWS_BATCH_JOB_MAIN_NODE_PRIVATE_IPV4_ADDRESS}:$HOST_FILE_PATH${AWS_BATCH_JOB_NODE_INDEX}
  do
    echo "Sleeping 1 seconds and trying again"
  done
  #echo "$ip slots=2" >> $HOST_FILE_PATH${AWS_BATCH_JOB_NODE_INDEX}
  SMTS/build/lemma_server -s ${AWS_BATCH_JOB_MAIN_NODE_PRIVATE_IPV4_ADDRESS}:3000
  ps -ef | grep sshd
  wait
}

#****************************************************Driver: send user request to appropriate function *****************
log $NODE_TYPE
case $NODE_TYPE in
  main)
    wait_for_nodes "${@}"
    ;;
  lemma)
    startLemmaServer "${@}"
    ;;
  child)
    report_to_master "${@}"
    ;;

  *)
    log $NODE_TYPE
    usage "Could not determine node type. Expected (main/child)"
    ;;
esac