#!/bin/bash

results=("$@")

# [[ -z $TIMEOUT ]] && TIMEOUT=300
[[ -z $TIMEOUT ]] && TIMEOUT=1200

IMAGE_FILE=plot.svg
PDF_FILE=plot.pdf
GNUPLOT_FILE=plot.gp

function cleanup {
  rm -f $GNUPLOT_FILE
}

python make_cactus_plot_gp.py -o "$IMAGE_FILE" -t $TIMEOUT "${results[@]}" >"$GNUPLOT_FILE" || {
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
