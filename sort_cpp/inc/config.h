#ifndef __CONFIG_H__
#define __CONFIG_H__

//#define BUFFER_SIZE         (1 << 10)       // current number of elements
                                            // per dpu

#define BLOCK_SIZE (BUFFER_SIZE / NR_TASKLETS)  // data to be sorted per thread

//#define CACHE_SIZE          (1 << 3)       // wram


#endif /* __CONFIG_H__ */
