#!/bin/bash
DIR=$(dirname $0)
OUTFILE=$(basename $0 .sh).out

rm -f $OUTFILE

for cache_log in {3..5}
do
    for arr_log in {15..20}
    do
        make -B -C $DIR BUFFER_SIZE=$((1<<$arr_log)) NR_TASKLETS=16 \
            CACHE_SIZE=$((1<<$cache_log)) || exit $?

        $DIR/host $((1<<$arr_log)) 0 >> $OUTFILE
    done
done