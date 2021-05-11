#ifndef __SORT_H__
#define __SORT_H__

#include <stdint.h>

int num_dpu_needed;

/* sort in_arr into out_arr */
void sort(uint32_t size, uint32_t *in_arr, uint32_t *out_arr);

#endif /* __SORT_H__ */