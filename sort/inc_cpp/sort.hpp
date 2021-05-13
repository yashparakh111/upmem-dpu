#ifndef __SORT_H__
#define __SORT_H__

#include <stdint.h>
#include <vector>

/* sort in_arr into out_arr */
void sort_pim(std::vector<uint32_t> in_arr, std::vector<uint32_t> &out_arr);

void allocate_dpus(uint32_t size);

void free_dpus(void);

#endif /* __SORT_H__ */