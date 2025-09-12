logic=$1
results_file=$2

dir=/home/tomaqa/smt-comp/benchmarks/storage/2025/non-incremental/$logic

while read line; do f="${line%% *}"; grep -q "^$dir/[-/a-zA-Z_0-9]*$f$" <data/${logic}_files_selected-200 && continue; echo "NOT IN FILES: >$dir/*/$f<"; break ; done <$results_file
