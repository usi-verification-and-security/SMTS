TACAS_SCRIPTS_DIR=$(dirname $(realpath "$0"))
ROOT_DIR=$(realpath "$TACAS_SCRIPTS_DIR/../../../")

[[ -z $os ]] && os='8'
[[ -z $nts ]] && nts='32'

# shopt -s extglob
# shopt -s patsub_replacement

function usage {
  echo "Usage: runme.sh <version_name> <files_file> [<start_line> <end_line>]"

  [[ -n $1 ]] && exit $1
}

function is_positive_int {
  [[ $1 =~ ^[1-9][0-9]*$ ]]
}

function check_is_positive_int {
  is_positive_int "$1" && return 0

  echo "Expected a positive number, got: $1" >&2
  usage 2 >&2
}

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

  [[ -n $new_files_file ]] && rm -f "$new_files_file"

  [[ -n $1 ]] && exit $1
}

trap 'cleanup 1' INT TERM QUIT

[[ -z $1 ]] && usage 0

version=$1

[[ -z $2 ]] && usage 1 >&2

files_file="$2"

[[ -n $3 ]] && {
  check_is_positive_int "$3"
  lo=$3
  if [[ -n $4 ]]; then
    check_is_positive_int "$4"
    hi=$4
    # new_files_file="${files_file/_files*(_+([0-9]))*(-+([0-9]))/&_${lo}-${hi}}"
    new_files_file="${files_file}_${lo}-${hi}"
    sed -n "${lo},${hi}p" <"$files_file" >"$new_files_file"
    files_file="$new_files_file"
  else
    echo "Expected <end_line>" >&2
    usage 1 >&2
  fi
}

files_name=$(basename "$files_file")

logic="${files_name%_files*}"

DATA_DIR=$(dirname "$files_file")
RESULTS_DIR="$DATA_DIR/${version}/${logic}"
mkdir -p "$RESULTS_DIR" >/dev/null || cleanup $?

[[ -z $TIMEOUT ]] && TIMEOUT=1200

[[ -z $N ]] && N=1

[[ -z $PARTITIONING ]] && PARTITIONING=1

[[ -z $LEMMA_SHARING ]] && LEMMA_SHARING=1

super_timeout=$(( (($TIMEOUT + 1) * 102) / 100 ))

err=$(mktemp)
out=$(mktemp)

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
    err_file=ERROR_$(basename "$results_file")
    while read file; do
      prev_res=
      sum_time=0
      ## make an average time, BUT just one timeout result implies average timeout
      for ((n=1; n<=$N; ++n)); do
        ## sometimes it hangs after finishing ... repeat several times until the case it does not
        for ((i=1; i<=3; ++i)); do
          timeout $super_timeout ./server/smts.py $partitioning_opt $lemma_sharing_opt -Pf -Pt $nt_opt -o $o -fp "$file" >$out 2>$err
          ret=$?
          sleep 0.5
          pkill solver_opensmt
          (( $ret == 0 )) && break
        done

        [[ -s $err ]] && {
          printf "ERROR output .err at %s:\n" "$file" >$err_file
          cleanup 1
        }

        if (( $ret != 0 )); then
          (( $ret != 124 )) && {
            printf "ERROR non-zero exit status at %s:\n" "$file" >$err_file
            cleanup 1
          }
          res=unknown
        else
          sed -i '/^;error/d' $out

          res=$(sed 's/^[^ ]* \([^ ]*\) .*$/\1/' <$out)
          [[ ! $res =~ ^(unsat|sat|unknown)$ ]] && {
            printf "ERROR at %s:\nUnrecognized result:\n%s\n" "$file" "$res" >$err_file
            cleanup 1
          }
        fi

        #++ distinguish mem-outs (use the time, not TIMEOUT)
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
    done <"$files_file" >"$results_file"
  done
done

cleanup 0
