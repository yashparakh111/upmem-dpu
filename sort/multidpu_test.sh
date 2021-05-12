#!/bin/bash
DIR=$(dirname $0)
OUTFILE=results/$(basename $0 .sh)
COUT=$OUTFILE.c.out
CPPOUT=$OUTFILE.cpp.out

rm -f $COUT
rm -f $CPPOUT 

for cache_log in {4..5}
do
    for arr_log in {10..15}
    do
        for buf_log in {8..10}
        do
            make -B -C $DIR BUFFER_SIZE=$((1<<$buf_log)) NR_TASKLETS=16 \
                CACHE_SIZE=$((1<<$cache_log)) || exit $?

            $DIR/host_c $((1<<$arr_log)) 0 >> $COUT
            $DIR/host_cpp $((1<<$arr_log)) 0 >> $CPPOUT
        done
    done
done