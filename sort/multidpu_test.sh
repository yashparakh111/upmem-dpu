#!/bin/bash
DIR=$(dirname $0)
OUTFILE=results/$(basename $0 .sh)
COUT=$OUTFILE.c.out
CPPOUT=$OUTFILE.cpp.out

rm -f $COUT
rm -f $CPPOUT 

for arr_log in {16..30}
do
    for buf_log in {12..24}
    do
        for cache_log in {8..8}
        do
            if (((arr_log - buf_log >= 4) && (arr_log - buf_log <= 8))); then
                make -B -C $DIR BUFFER_SIZE=$((1<<$buf_log)) NR_TASKLETS=16 \
                    CACHE_SIZE=$((1<<$cache_log)) || exit $?

                #$DIR/host_c $((1<<$arr_log)) 0 >> $COUT
                $DIR/host_cpp $((1<<$arr_log)) 0 >> $CPPOUT
            fi
        done
    done
done