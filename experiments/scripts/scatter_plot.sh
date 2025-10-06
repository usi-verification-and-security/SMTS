#!/bin/bash

results1=$1
results2=$2

# [[ -z $TIMEOUT ]] && TIMEOUT=300
[[ -z $TIMEOUT ]] && TIMEOUT=1200

IMAGE_FILE=plot.svg
PDF_FILE=plot.pdf
GNUPLOT_FILE=plot.gp

function cleanup {
  rm -f $GNUPLOT_FILE
}

python make_scatter_plot_gp.py "$results1" "$results2" $results1 $results2 "" '' "$IMAGE_FILE" $TIMEOUT_SECONDS >"$GNUPLOT_FILE" || {
    status=$?
    [[ -s $GNUPLOT_FILE ]] && less "$GNUPLOT_FILE"
    cleanup
    exit $status
}

gnuplot "$GNUPLOT_FILE" || exit $?

# printf "The resulting plot is stored in %s\n" "$IMAGE_FILE"

display "$IMAGE_FILE"

inkscape --export-filename="$PDF_FILE" "$IMAGE_FILE"

cleanup
exit 0
