#!/bin/bash
DIR=$(dirname $0)
OUTFILE=results/$(basename $0 .sh)
COUT=$OUTFILE.c.out
CPPOUT=$OUTFILE.cpp.out

rm -f $COUT
rm -f $CPPOUT 

min_dpu_log=4  # 16 dpus
max_dpu_log=8  # 256 dpus

for buf_log in {12..24}
do
    make -B -C $DIR BUFFER_SIZE=$((1<<$buf_log)) NR_TASKLETS=16 \
        CACHE_SIZE=256 || exit $?
    #$DIR/host_c $((1<<$arr_log)) 0 >> $COUT
    $DIR/host_cpp $(($buf_log+$min_dpu_log)) $(($buf_log+$max_dpu_log)) 0 >> $CPPOUT
done
