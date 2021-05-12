#!/bin/bash
DIR=$(dirname $0)
OUTFILE=results/$(basename $0 .sh)
COUT=$OUTFILE.c.out
CPPOUT=$OUTFILE.cpp.out

rm -f $COUT
rm -f $CPPOUT 

for arr_log in {16..25}
do
    for buf_log in {12..20}
    do
        for cache_log in {8..8}
        do
            if (((arr_log - buf_log >= 4) && (arr_log - buf_log <= 8))); then
                make -B -C $DIR BUFFER_SIZE=$((1<<$buf_log)) NR_TASKLETS=16 \
                    CACHE_SIZE=$((1<<$cache_log)) || exit $?

                #$DIR/host_c $((1<<$arr_log)) 0 >> $COUT
                dpu-profiling memory-transfer -- $DIR/host_cpp $((1<<$arr_log)) 0 >> perf/mem/$arr_log.$buf_log.$cache_log.out
                # dpu-profiling functions -f "APPLICATION_FUNCTIONS" -d "DPU_HOST_API_FUNCTIONS" -- $DIR/host_cpp $((1<<$arr_log)) 0 >> perf/func/$arr_log.$buf_log.$cache_log.out
                # dpu-profiling functions -o perf/traces/$arr_log.$buf_log.$cache_log.json -A -- $DIR/host_cpp $((1<<$arr_log)) 0 
            fi
        done
    done
done