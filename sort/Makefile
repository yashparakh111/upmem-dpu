NR_TASKLETS ?= 1
BUILD_DIR = build
INC_DIR = inc
SRC_DIR = src

INCS := $(wildcard $(INC_DIR)/*.h)

CC := gcc
CFLAGS := -std=c99 -I$(INC_DIR) -DNR_TASKLETS=$(NR_TASKLETS)

.PHONY: all clean

all: host

host: $(SRC_DIR)/host.c $(SRC_DIR)/sort.c $(INCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/host.c $(SRC_DIR)/sort.c `dpu-pkg-config --cflags --libs dpu`

$(BUILD_DIR)/radix_sort: $(SRC_DIR)/radix_sort.c $(INC_DIR)/config.h
	dpu-upmem-dpurte-clang -O2 -Iinc -DNR_TASKLETS=$(NR_TASKLETS) -o $@ $(SRC_DIR)/radix_sort.c

$(BUILD_DIR):
	@mkdir $(BUILD_DIR) || true

clean:
	rm -rf $(BUILD_DIR) host
