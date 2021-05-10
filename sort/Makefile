NR_TASKLETS ?= 1
BUILD_DIR = build

.PHONY: all clean

all: $(BUILD_DIR)/host $(BUILD_DIR)/radix_sort

$(BUILD_DIR)/host: host.c config.h
	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
	gcc --std=c99 -DNR_TASKLETS=$(NR_TASKLETS) host.c -o host `dpu-pkg-config --cflags --libs dpu`

$(BUILD_DIR)/radix_sort: radix_sort.c config.h
	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
	dpu-upmem-dpurte-clang -O2 -DNR_TASKLETS=$(NR_TASKLETS) -o $(BUILD_DIR)/radix_sort radix_sort.c

clean:
	rm -rf $(BUILD_DIR) host