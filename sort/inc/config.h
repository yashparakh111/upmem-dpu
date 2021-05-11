#ifndef __CONFIG_H__
#define __CONFIG_H__

#define BUFFER_SIZE         128     // current number of elements
                                    // per dpu

#define MAX_BUFFER_SIZE     256     // defines maximum number of
                                    // elements any dpu can hold

#define CACHE_SIZE          8       // wram

#define BLOCK_SIZE (BUFFER_SIZE / NR_TASKLETS)

#endif /* __CONFIG_H__ */
