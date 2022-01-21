#!/bin/bash

if [ $# != 3 ]; then
    echo "Usage: $0 <provide benchs dir>"
    exit 1
fi
function get_abs_path {
  echo $(cd $(dirname $1); pwd)/$(basename $1)
}

SCRIPT_ROOT=$(get_abs_path $(dirname $0))
#echo $SCRIPT_ROOT
total=0
bashstr=''


name=$(basename $1)
n_benchmarks=$(find $1 -name '*.smt2' | wc -l)

port=3000
echo $n_benchmarks
gname=$name-$(date +'%F')
scriptdir=$SCRIPT_ROOT'/packed/'$name'/'
outd=$SCRIPT_ROOT'/result/'$name'/'
mkdir -p ${scriptdir}

echo "Benchmark set (total ${n_benchmarks}):"
rm $outd$name-'remained'
((n_node=((n_benchmarks/($2*4)))))
echo "N Benchmark: ${n_benchmarks}"  "- N Node:  ${n_node}" - 'N bench per server:' $2 >> $outd$name-'remained'

echo "Number of Nodes (total ${n_node}):"
#n_remained=n_node-1
find $1 -name '*.smt2' | while read -r file;
  do

    ((total=total+1))
#    if  [ ${total} -gt $((n_benchmarks-n_remained)) ]
#          then
#            echo $file >> $outd$name-'remained'
#    fi
    if  [ ${total} == $2 ]
        then
          filepaths+="'$file'"
       else
          filepaths+="'$file'",
     fi

    if  [ ${total} == $2 ]
      then

        if  [ ${port} == 3004 ]
          then
            port=3000

            echo $SCRIPT_ROOT'/packed/'$gname''-$n_node'.sh'
#            echo "Send for Node" ${n_node}
            n_node=$((n_node-1))
#            sleep 0.3
        fi

        command=$3'smts.py -o3 -p '$port' -fp '
        if  [ ${port} == 3003 ]
          then
            bashstr+="$command""$filepaths" ;
            bashstr+=" & wait "
#              echo $bashstr
            ex=$1;
            bname=`basename $ex`
            scrname=$SCRIPT_ROOT'/packed/'$name'/'$gname''-$n_node'.sh'
#            echo $scrname
            cat << _EOF_ > $scrname

output=$outd

_EOF_

#          i=$((i+1))
          cat << _EOF_ >> $scrname
 (
    chmod +x $scrname
    $bashstr
 ) > \$output/${gname}-$n_node.out & wait
_EOF_

        #    echo "wait" >> $packedscrd/$scrname
#            echo $packedscrd
#            echo $scrname

            bashstr=''
          else
            bashstr+="$command""$filepaths" ;
            bashstr+=" & "
        fi

        filepaths=''
        total=0
        ((port=port+1))
    fi

  done
echo "Construct and send the above jobs to batch queue?"
read -p "y/N? "

if [[ ${REPLY} != y ]]; then
    echo "Aborting."
    exit 1
fi

mkdir -p $outd
#${WORKSCRIPT} ${smtServer} ${scriptdir} ${resultdir} ${config} ${bmpath}/*.smt2.bz2
for script in ${scriptdir}/*.sh; do
    echo ${script};
    sh ${script};
    sleep 1;
done



