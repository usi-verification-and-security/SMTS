#!/bin/bash

if [ $# -eq 0 ]; then 
	echo "Usage $0 <result-file>"
	exit 1;
fi

result=$1
echo $result
name=$(basename $result)-child

tmpdir=$(mktemp -d)
echo $tmpdir
#trap "rm -rf $tmpdir" EXIT

for type in blind tterm bterm noteq eq tterm_neq portfolio; do
	tail +2 ${result} \
		|grep $type \
		|grep 'root$' \
		|awk '{print $3}' \
		|sort -n \
		|nl >$tmpdir/$type.list
done

(
	echo "set term pngcairo"
	echo "set output 'cactus-plot-"$name".png'"
	echo "set logscale x"
	echo "set logscale y"
	echo "set xlabel 'time'"
	echo "set ylabel 'number of instances'"
	echo -n "plot "
	for type in blind tterm bterm noteq eq tterm_neq portfolio; do
		echo -n "\"$tmpdir/$type.list\" using 2:1 title "\"$type\"", "
	done
	echo
) |gnuplot
