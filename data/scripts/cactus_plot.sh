#!/bin/bash

results=("$@")

TIMEOUT_SECONDS=300
# TIMEOUT_SECONDS=600

IMAGE_FILE=plot.png
GNUPLOT_FILE=plot.gp

function cleanup {
  rm -f $GNUPLOT_FILE
}

python make_cactus_plot_gp.py -o "$IMAGE_FILE" -t $TIMEOUT_SECONDS "${results[@]}" >"$GNUPLOT_FILE" || {
    status=$?
    [[ -s $GNUPLOT_FILE ]] && less "$GNUPLOT_FILE"
    cleanup
    exit $status
}

gnuplot "$GNUPLOT_FILE" || exit $?

# printf "The resulting plot is stored in %s\n" "$IMAGE_FILE"

display "$IMAGE_FILE"

cleanup
exit 0
