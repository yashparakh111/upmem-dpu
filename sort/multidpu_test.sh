#!/bin/bash
DIR=$(dirname $0)
OUTFILE=results/$(basename $0 .sh)
COUT=$OUTFILE.c.out
CPPOUT=$OUTFILE.cpp.out

rm -f $COUT
rm -f $CPPOUT 

for arr_log in {16..23}
do
    for buf_log in {11..16}
    do
        for cache_log in {8..8}
        do
            #if ((buf_log - cache_log == ??)); then
            make -B -C $DIR BUFFER_SIZE=$((1<<$buf_log)) NR_TASKLETS=16 \
                CACHE_SIZE=$((1<<$cache_log)) || exit $?

            #$DIR/host_c $((1<<$arr_log)) 0 >> $COUT
            $DIR/host_cpp $((1<<$arr_log)) 0 >> $CPPOUT
            #fi
        done
    done
done