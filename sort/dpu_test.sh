#!/bin/bash
DIR=$(dirname $0)
OUTFILE=results/$(basename $0 .sh)
COUT=$OUTFILE.c.out
CPPOUT=$OUTFILE.cpp.out

rm -f $COUT
rm -f $CPPOUT 

arr_log_min=10
arr_log_max=15
for cache_log in {3..5}
do
    make -B -C $DIR BUFFER_SIZE=$((1<<$arr_log_min)) NR_TASKLETS=16 \
        CACHE_SIZE=$((1<<$cache_log)) || exit $?

    #$DIR/host_c $((1<<$arr_log)) 0 >> $COUT
    $DIR/host_cpp $arr_log_min $arr_log_max 0 >> $CPPOUT
done