#!/usr/bin/env bash

set -x

aws --profile $1 ecs run-task --launch-type EC2 --cluster $2 --task-definition $3 --network-configuration \
"{
   \"awsvpcConfiguration\": {
      \"subnets\": [\"$4\"],
      \"securityGroups\": [\"$5\"]
   }
}" \
--overrides \
"{
  \"containerOverrides\": [{
    \"name\": \"$9\",
    \"environment\": [
        {
            \"name\":\"AWS_BATCH_JOB_NODE_INDEX\",
            \"value\":  \"$6\"
        },
        {
            \"name\":\"NUM_PROCESSES\",
            \"value\": \"$7\"
        },
        {
            \"name\":\"AWS_BATCH_JOB_NUM_NODES\",
            \"value\": \"$7\"
        },
        {
            \"name\":\"S3_BKT\",
            \"value\":\"smt-comp-2021\"
        },
        {
            \"name\":\"AWS_BATCH_JOB_MAIN_NODE_PRIVATE_IPV4_ADDRESS\",
            \"value\":\"$8\"
        }
        ,
        {
            \"name\":\"test\",
            \"value\":\"$$10\"
        }
    ]
  }]
}
"
