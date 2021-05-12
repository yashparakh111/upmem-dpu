#!/bin/bash
DIR=$(dirname $0)
OUTFILE=results/$(basename $0 .sh).out

rm -f $OUTFILE
g++ -O3 -std=c++17 -o build/sort_cpp src/sort_tests.cpp -ltbb

for arr_log in {10..24}
do
    $DIR/build/sort_cpp $((1 << $arr_log)) >> $OUTFILE
done