logic=$1
results_file=$2

while read line; do f="${line##*/}"; grep -q "^$f " <$results_file && continue; echo "NE: >$f<"; break ; done <data/${logic}_files_selected-200
