#!/bin/bash
DIR=$(dirname $0)
OUTFILE=$(basename $0 .sh).out

rm -f $OUTFILE
gcc -std=c99 -o sort sort_tests.c 

for arr_log in {10..17}
    do
        $DIR/sort $((1 << $arr_log)) >> $OUTFILE
    done
done