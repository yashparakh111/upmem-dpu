/*	
 * Sorting algorithm:
 * 1) Merge sort on host side splits up the data to be sorted into chunks. Each chunk is sent to a single dpu to be sorted via radix sort.
 * 2) PIM algorithm to perform radix-2^b sort(where b in the number of bits being grouped):
 * 		a) Bucket each value based on digit. Thread per bucket can be parallelized
 *		b) Parallel-prefix sum on buckets using a thread per bucket.
 *		c) Scatter the input data using a thread per bucket. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <perfcounter.h>

#include <mram.h>
#include <defs.h>
#include <barrier.h>

#include "config.h"

#define CACHE_SIZE 16
#define RADIX_LOG2 4
#define RADIX (1 << RADIX_LOG2)
#define VAL_SIZE 32

// number of elements each tasklet is responsible for sorting
#define BLOCK_SIZE (BUFFER_SIZE / NR_TASKLETS)

BARRIER_INIT(my_barrier, NR_TASKLETS);

__mram_noinit uint32_t mem[BUFFER_SIZE];				  // mram
__dma_aligned uint32_t cache[2][NR_TASKLETS][CACHE_SIZE]; // wram

void populate_data()
{
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		mem[i] = (BUFFER_SIZE - i) % 49;
	}
}

int main()
{
	int tid = me();
	uint32_t buckets[RADIX];
	//uint8_t* buckets = mem_alloc(sizeof(uint8_t) * RADIX);

	/*** Start perf count ***/
	if (tid == 0)
	{
		// populate_data();
		// print_data();
		perfcounter_config(COUNT_CYCLES, true);
	}
	barrier_wait(&my_barrier);
	/************************/

	uint32_t cache_type = 0;

	// loop through each thread's block and sort it at a CACHE_SIZE granularity
	for (int t = tid * BLOCK_SIZE; t < (tid + 1) * BLOCK_SIZE; t += CACHE_SIZE)
	{
		// copy small part into the cache
		mram_read(&mem[t], &cache[cache_type][tid][0], sizeof(uint32_t) * CACHE_SIZE);

		// sort on each digit group
		for (int d = 0; d < VAL_SIZE; d += RADIX_LOG2)
		{
			// reset the buckets
			for (int b = 0; b < RADIX; b++)
			{
				buckets[b] = 0;
			}

			// bucket each value
			for (int i = 0; i < CACHE_SIZE; i++)
			{
				uint32_t bid = (cache[cache_type][tid][i] >> d) & (RADIX - 1);
				buckets[bid]++;
			}

			// do prefix sum on the buckets
			for (int b = 1; b < RADIX; b++)
			{
				buckets[b] += buckets[b - 1];
			}

			// scatter the values
			for (int i = CACHE_SIZE - 1; i >= 0; i--)
			{
				uint32_t bid = (cache[cache_type][tid][i] >> d) & (RADIX - 1);
				cache[cache_type ^ 1][tid][--buckets[bid]] = cache[cache_type][tid][i];
			}

			// swap sorted and unsorted caches
			cache_type ^= 1;
		}
		
		// write back sorted part
		mram_write(&cache[cache_type][tid][cache_type], &mem[t], sizeof(uint32_t) * CACHE_SIZE);
	}

	/*** End perf count ***/
	barrier_wait(&my_barrier);
	if (tid == 0)
	{
		// print results
		perfcounter_t end_time = perfcounter_get();
		// printf("%lu\n", end_time);

		// print_data();
	}
	/************************/

	return 0;
}
