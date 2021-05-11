NR_TASKLETS ?= 1
BUILD_DIR = build
INC_DIR = inc
SRC_DIR = src

CC := gcc
CFLAGS := -std=c99 -I$(INC_DIR) -DNR_TASKLETS=$(NR_TASKLETS) -Iinc

.PHONY: all clean

all: host $(BUILD_DIR)/radix_sort

host: $(SRC_DIR)/host.c $(SRC_DIR)/sort.c $(INC_DIR)/*.h
	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/host.c $(SRC_DIR)/sort.c `dpu-pkg-config --cflags --libs dpu`

# $(BUILD_DIR)/host.o: host.c $(INC_DIR)/config.h $(INC_DIR)/sort.h
# 	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
# 	$(CC) $(CFLAGS) -c -o $@ host.c

# $(BUILD_DIR)/sort.o: sort.c $(INC_DIR)/config.h $(INC_DIR)/sort.h
# 	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
# 	$(CC) $(CFLAGS) -c -o $@ sort.c `dpu-pkg-config --cflags --libs dpu`

$(BUILD_DIR)/radix_sort: $(SRC_DIR)/radix_sort.c $(INC_DIR)/config.h
	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
	dpu-upmem-dpurte-clang -O2 -Iinc -DNR_TASKLETS=$(NR_TASKLETS) -o $@ $(SRC_DIR)/radix_sort.c

clean:
	rm -rf $(BUILD_DIR) host