NR_TASKLETS ?= 1

.PHONY: all clean

all: host radix_sort

host: host.c config.h
	gcc --std=c99 host.c -o host `dpu-pkg-config --cflags --libs dpu`

radix_sort: radix_sort.c config.h
	dpu-upmem-dpurte-clang -O2 -DNR_TASKLETS=$(NR_TASKLETS) -o radix_sort radix_sort.c

clean:
	rm host radix_sort