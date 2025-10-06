SCRIPTS_DIR=$(dirname $(realpath "$0"))
ROOT_DIR=$(realpath "$SCRIPTS_DIR/../../../")

[[ -z $os ]] && os='8'
[[ -z $nts ]] && nts='32'

function usage {
  echo "Usage: runme.sh <version_name> <files_file>"

  [[ -n $1 ]] && exit $1
}

[[ -z $1 ]] && usage 0

version=$1

files_file="$2"
files_name=$(basename "$files_file")

DATA_DIR=$(dirname "$files_file")
RESULTS_DIR="$DATA_DIR/$version/$files_name"
mkdir -p "$RESULTS_DIR" >/dev/null || exit $?

[[ -z $TIMEOUT ]] && TIMEOUT=300

[[ -z $N ]] && N=1

[[ -z $PARTITIONING ]] && PARTITIONING=1

[[ -z $LEMMA_SHARING ]] && LEMMA_SHARING=1

super_timeout=$(( $TIMEOUT + 5 ))

err=$(mktemp)
out=$(mktemp)

function cleanup {
  if [[ -n $1 && $1 != 0 ]]; then
    if [[ -n $err_file ]]; then
      [[ -r $out ]] && mv $out ${err_file}.out
      [[ -r $err ]] && mv $err ${err_file}.err
    else
      [[ -r $out ]] && mv $out ERROR_out
      [[ -r $err ]] && mv $err ERROR_err
    fi
  else
    rm -f $err
    rm -f $out
  fi

  [[ -n $1 ]] && exit $1
}

trap 'cleanup 1' INT TERM QUIT

cd "$ROOT_DIR"

if (( $PARTITIONING )); then
  partitioning_opt='-p'
else
  unset partitioning_opt
fi

for o in $os; do
  if (( $o > 1 && $LEMMA_SHARING )); then
    lemma_sharing_opt='-l'
  else
    unset lemma_sharing_opt
  fi

  for nt in $nts; do
    results_file="$RESULTS_DIR/${version}_${files_name}${partitioning_opt}${lemma_sharing_opt}"
    if [[ $nt == 0 ]]; then
      unset nt_opt
    else
      results_file+="_nt-${nt}"
      nt_opt="-nt $nt"
    fi
    results_file+="_o-${o}"
    err_file=ERROR_$(basename $results_file)
    while read file; do
      prev_res=
      sum_time=0
      for ((n=1; n<=$N; ++n)); do
        while true; do
          timeout $super_timeout ./server/smts.py $partitioning_opt $lemma_sharing_opt -Pf -Pt $nt_opt -o $o -fp "$file" >$out 2>$err
          ret=$?
          sleep 0.5
          (( $ret == 0 )) && break
        done

        sed -i '/^;error/d' $out

        res=$(sed 's/^[^ ]* \([^ ]*\) .*$/\1/' <$out)
        [[ ! $res =~ ^(unsat|sat|unknown)$ || -s $err ]] && {
          printf "ERROR at %s:\nUnrecognized result:\n%s\n" "$file" "$res" >$err_file
          cleanup 1
        }
        [[ $res == unknown ]] && {
          printf "%s %s %s\n" "$file" $res $TIMEOUT
          break
        }
        [[ -n $prev_res && $res != $prev_res ]] && {
          printf "ERROR: result mismatch: %s != %s\nat: %s\n" $res $prev_res "$file" >$err_file
          cleanup 1
        }
        prev_res=$res

        sum_time+='+'$(sed 's/^.* \([0-9.]*\)$/\1/' <$out)
      done
      [[ $res == unknown ]] && continue
      avg_time=$(bc -l <<<"scale=2; ($sum_time)/$N")
      printf "%s %s %s\n" "$file" $res $avg_time
    done <$files_file >$results_file
  done
done

cleanup 0
