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
#include <seqread.h>

#include "config.h"

#define RADIX_LOG2 4
#define RADIX (1 << RADIX_LOG2)
#define VAL_SIZE 32

// number of elements each tasklet is responsible for sorting

BARRIER_INIT(my_barrier, NR_TASKLETS);

__mram_noinit uint32_t mem[BUFFER_SIZE];
__dma_aligned uint32_t cache[2][NR_TASKLETS][CACHE_SIZE]; // wram

void print_arr(char *name, __mram_ptr uint32_t *arr, int size)
{
    printf("%s: ", name);
    for (int i = 0; i < size; i++)
    {
        printf("%02u ", arr[i]);
    }
    printf("\n\n");
}

void merge(uint32_t *unmerged_wram,
			uint32_t *merge_buf_wram, 
			__mram_ptr uint32_t *merged_mram,
			__mram_ptr uint32_t *empty_mram,
			uint32_t num_merged_chunks
			)
{
	
	seqreader_buffer_t local_cache;
	seqreader_t sr;

	uint32_t merged_size, unmerged_size;
	uint32_t merged_val, unmerged_val;

	// initialize seqreader
	local_cache = seqread_alloc();
	uint32_t *merged_reader = seqread_init(local_cache, merged_mram, &sr);

	unmerged_size = CACHE_SIZE;
	merged_size = CACHE_SIZE * num_merged_chunks;

	// merge upto a CACHE_SIZE values and write back to empty_mram
	// repeat num_merged_chunks + 1 number of times
	for (int c = 0; c < num_merged_chunks + 1; c++)
	{
		// fill up merge_buffer by merging unmerged_wram
		// and merged_mram at CACHE_SIZE granularity
		for (int i = 0; i < CACHE_SIZE; i++)
		{
			if (merged_size != 0)
			{
				merged_val = *merged_reader;
			}
			if (unmerged_size != 0)
			{
				unmerged_val = *unmerged_wram;
			}

			if ((merged_size == 0) || ((unmerged_size != 0) && (merged_val > unmerged_val)))
			{
				merge_buf_wram[i] = unmerged_val;
				unmerged_wram++;
				unmerged_size--;
			}
			else if ((unmerged_size == 0) || (unmerged_val >= merged_val))
			{
				merge_buf_wram[i] = merged_val;
				merged_reader = seqread_get(merged_reader, sizeof(uint32_t), &sr);
				merged_size--;
			}
			// if(merge_buf_wram[i] == 0) {
			// 	printf("%d %d -- %u %u \n", merged_size, unmerged_size, *merged_reader, unmerged_val);
			// }
		}

		// write merge_buf to empty
		mram_write(merge_buf_wram, empty_mram, sizeof(uint32_t) * CACHE_SIZE);

		// increment empty by CACHE_SIZE
		empty_mram += CACHE_SIZE;
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
	uint32_t num_merged_chunks = 0;

	// loop through each thread's block and sort it at a CACHE_SIZE granularity and perform merge
	for (int c = ((tid + 1) * BLOCK_SIZE) - CACHE_SIZE; c >= tid * BLOCK_SIZE; c -= CACHE_SIZE)
	{
		// copy small part into the cache
		mram_read(&mem[c], &cache[cache_type][tid][0], sizeof(uint32_t) * CACHE_SIZE);

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

		// n-way merge
		merge(cache[cache_type][tid], cache[cache_type ^ 1][tid], &mem[c + CACHE_SIZE], &mem[c], num_merged_chunks);
		num_merged_chunks++;

		// write back sorted part
		//mram_write(&cache[cache_type][tid][cache_type], &mem[c], sizeof(uint32_t) * CACHE_SIZE);
	}

	barrier_wait(&my_barrier);


	// merge sorted blocks
	


	/*** End perf count ***/
	barrier_wait(&my_barrier);
	if (tid == 0)
	{
		// print_arr("arr", mem, BUFFER_SIZE);

		// print results
		perfcounter_t end_time = perfcounter_get();
		// printf("%lu\n", end_time);

		// print_data();
	}
	/************************/

	return 0;
}
