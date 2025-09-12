function usage {
  echo "$0 <files> <results>"
  [[ -n $1 ]] && exit $1
}

[[ -z $1 ]] && usage 1
[[ -z $2 ]] && usage 1

files=$1
results=$2

results_out=${results}_with_paths

n=$(wc -l <$files)
for ((ln=1; $ln <= $n; ++ln )); do
 file=$(sed -n "${ln}p" $files)
 sed -n "${ln}p" $results | awk "{ printf(\"%s %s %s\n\", \"$file\", \$2, \$3) }"
done >$results_out
