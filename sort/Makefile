NR_TASKLETS ?= 1
CACHE_SIZE ?= 8
BUFFER_SIZE ?= 128
BUILD_DIR = build

INC_DIR = inc
SRC_DIR = src

INC_DIR_CPP = inc_cpp
SRC_DIR_CPP = src_cpp

INCS_C := $(wildcard $(INC_DIR)/*)
INCS_CPP := $(wildcard $(INC_DIR_CPP)/*)

CC := gcc
CFLAGS := -std=c99 -I$(INC_DIR)

CPP := g++
CPPFLAGS := -std=c++17 -I$(INC_DIR_CPP)


DEFS := -DNR_TASKLETS=$(NR_TASKLETS) -DCACHE_SIZE=$(CACHE_SIZE) -DBUFFER_SIZE=$(BUFFER_SIZE)


.PHONY: all clean

all: host_c host_cpp

host_c: $(SRC_DIR)/host.c $(SRC_DIR)/sort.c $(BUILD_DIR)/dpu_sort $(INCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DEFS) -o $@ $(SRC_DIR)/host.c $(SRC_DIR)/sort.c `dpu-pkg-config --cflags --libs dpu`

host_cpp: $(SRC_DIR_CPP)/host.cpp $(SRC_DIR_CPP)/sort.cpp $(BUILD_DIR)/dpu_sort $(INCS_CPP) | $(BUILD_DIR)
	$(CPP) $(CPPFLAGS) $(DEFS) -o $@ $(SRC_DIR_CPP)/host.cpp $(SRC_DIR_CPP)/sort.cpp `dpu-pkg-config --cflags --libs dpu`

$(BUILD_DIR)/dpu_sort: $(SRC_DIR)/dpu_sort.c $(INC_DIR)/config.h | $(BUILD_DIR)
	dpu-upmem-dpurte-clang -O2 -Iinc -DSTACK_SIZE_DEFAULT=256 $(DEFS) -o $@ $(SRC_DIR)/dpu_sort.c

$(BUILD_DIR):
	@mkdir $(BUILD_DIR) || true

clean:
	rm -rf $(BUILD_DIR) host
