#!/bin/bash
DIR=$(dirname $0)
OUTFILE=results/$(basename $0 .sh).out

rm -f $OUTFILE
gcc -O3 -o build/sort src/sort_tests.c 

for arr_log in {10..24}
do
    $DIR/build/sort $((1 << $arr_log)) >> $OUTFILE
done