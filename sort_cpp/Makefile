NR_TASKLETS ?= 1
CACHE_SIZE ?= 8
BUFFER_SIZE ?= 128
BUILD_DIR = build
INC_DIR = inc
SRC_DIR = src

INCS := $(wildcard $(INC_DIR)/*.h)

CC := g++
CFLAGS := -std=c++17 -I$(INC_DIR)
DEFS := -DNR_TASKLETS=$(NR_TASKLETS) -DCACHE_SIZE=$(CACHE_SIZE) -DBUFFER_SIZE=$(BUFFER_SIZE)


.PHONY: all clean

all: host

host: $(SRC_DIR)/host.cpp $(SRC_DIR)/sort.cpp $(BUILD_DIR)/radix_sort $(INCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DEFS) -o $@ $(SRC_DIR)/host.cpp $(SRC_DIR)/sort.cpp `dpu-pkg-config --cflags --libs dpu`

$(BUILD_DIR)/radix_sort: $(SRC_DIR)/radix_sort.c $(INC_DIR)/config.h | $(BUILD_DIR)
	dpu-upmem-dpurte-clang -O2 -Iinc $(DEFS) -o $@ $(SRC_DIR)/radix_sort.c

$(BUILD_DIR):
	@mkdir $(BUILD_DIR) || true

clean:
	rm -rf $(BUILD_DIR) host
