#!/bin/bash
DIR=$(dirname $0)
OUTFILE=r$(basename $0 .sh)
COUT=$OUTFILE.c.out
CPPOUT=$OUTFILE.cpp.out

rm -f $COUT
rm -f $CPPOUT 

for cache_log in {3..5}
do
    for arr_log in {10..15}
    do
        make -B -C $DIR BUFFER_SIZE=$((1<<$arr_log)) NR_TASKLETS=16 \
            CACHE_SIZE=$((1<<$cache_log)) || exit $?

        $DIR/host_c $((1<<$arr_log)) 0 >> $COUT
        $DIR/host_cpp $((1<<$arr_log)) 0 >> $CPPOUT
    done
done