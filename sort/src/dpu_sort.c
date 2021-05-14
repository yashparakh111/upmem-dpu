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
#include <alloc.h>

#include <mram.h>
#include <defs.h>
#include <barrier.h>
#include <seqread.h>

#include "config.h"

#define RADIX_LOG2 4
#define RADIX (1 << RADIX_LOG2)
#define VAL_SIZE 32

#define MAX_MRAM_TRANSFER 2048
#define MRAM_READ 1
#define MRAM_WRITE 0

// number of elements each tasklet is responsible for sorting

BARRIER_INIT(my_barrier, NR_TASKLETS);

__mram_noinit uint32_t mem[BUFFER_SIZE];
__dma_aligned uint32_t cache[2][NR_TASKLETS][CACHE_SIZE]; // wram
__host uint32_t execution_cycles;
__host double execution_time;

// void print_arr(char *name, __mram_ptr uint32_t *arr, int size)
// {
//     printf("%s: ", name);
//     for (int i = 0; i < size; i++)
//     {
//         printf("%02u ", arr[i]);
//     }
//     printf("\n\n");
// }

seqreader_buffer_t local_cache[NR_TASKLETS];
seqreader_t sr[NR_TASKLETS];

// copy from mram into wram
// need to respect mram max transfer limit of MRAM_MAX_TRANSFER bytes
// therefore the transfer happens in MRAM_MAX_TRANSFER chunks
void cache_transfer(int is_read,
					uint32_t *wram_from,
					__mram_ptr uint32_t *mram_to,
					__mram_ptr uint32_t *mram_from,
					uint32_t *wram_to,
					uint32_t num_bytes)
{

	uint32_t cache_addr = 0;
	uint32_t byte_amount = 0;

	// cast the correct array to mram type
	while (num_bytes > 0)
	{
		byte_amount = num_bytes >= MAX_MRAM_TRANSFER ? MAX_MRAM_TRANSFER : num_bytes;
		if (is_read)
			mram_read(mram_from, wram_to, byte_amount);
		else
			mram_write(wram_from, mram_to, byte_amount);
		cache_addr += byte_amount / sizeof(uint32_t);
		num_bytes -= byte_amount;
	}
}

void merge(int tid,
		   uint32_t *unmerged_wram,
		   uint32_t *merge_buf_wram,
		   __mram_ptr uint32_t *merged_mram,
		   __mram_ptr uint32_t *empty_mram,
		   uint32_t num_merged_chunks)
{

	uint32_t merged_size, unmerged_size;
	uint32_t merged_val, unmerged_val;

	// initialize seqreader
	uint32_t *merged_reader = seqread_init(local_cache[tid], merged_mram, &(sr[tid]));

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
				merged_reader = seqread_get(merged_reader, sizeof(uint32_t), &(sr[tid]));
				merged_size--;
			}
		}

		// write merge_buf to empty
#if (CACHE_SIZE * 4) <= MAX_MRAM_TRANSFER
		mram_write(merge_buf_wram, empty_mram, sizeof(uint32_t) * CACHE_SIZE);
#else
		cache_transfer(MRAM_WRITE, merge_buf_wram, empty_mram, NULL, NULL, sizeof(uint32_t) * CACHE_SIZE);
#endif

		// if unmerged has been written out, nothing else to merge
		if (unmerged_size == 0)
			return;

		// increment empty by CACHE_SIZE
		empty_mram += CACHE_SIZE;

		
	}
}

void radix_sort(int tid, uint32_t *cache_type_ptr)
{
	uint32_t buckets[RADIX];
	uint32_t cache_type = *cache_type_ptr;

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

	*cache_type_ptr = cache_type;
}

void insertion_sort(uint32_t *my_cache)
{
	for (int i = 1; i < CACHE_SIZE; i++)
	{
		int j = i - 1;
		while (j >= 0 && my_cache[j] > my_cache[j + 1])
		{
			uint32_t swap_val = my_cache[j];
			my_cache[j] = my_cache[j + 1];
			my_cache[j + 1] = swap_val;
			j--;
		}
	}
}

int main()
{
	/*** Start perf count ***/
	// if (tid == 0)
	// {
	// populate_data();
	// print_data();
	// 	perfcounter_config(COUNT_CYCLES, true);
	// }

	// barrier_wait(&my_barrier);
	/************************/

	int tid = me();
	
    if(tid == 0){
        mem_reset();
    }

    barrier_wait(&my_barrier);

    uint32_t num_merged_chunks = 0;
	uint32_t cache_type = 0;

	local_cache[tid] = seqread_alloc();

	// loop through each thread's block and sort it at a CACHE_SIZE granularity and perform merge
	for (int c = ((tid + 1) * BLOCK_SIZE) - CACHE_SIZE; c >= tid * BLOCK_SIZE; c -= CACHE_SIZE)
	{
#if (CACHE_SIZE * 4) <= MAX_MRAM_TRANSFER
		mram_read(&mem[c], &cache[cache_type][tid][0], sizeof(uint32_t) * CACHE_SIZE);
#else
		cache_transfer(MRAM_READ, NULL, NULL, &mem[c], &cache[cache_type][tid][0], CACHE_SIZE * sizeof(uint32_t));
#endif

#ifdef USE_INSERTION_SORT
		insertion_sort(cache[cache_type][tid]);
#else
		radix_sort(tid, &cache_type);
#endif
		// n-way merge
		merge(tid, cache[cache_type][tid], cache[cache_type ^ 1][tid], &mem[c + CACHE_SIZE], &mem[c], num_merged_chunks);
		num_merged_chunks++;
	}

#ifdef DPU_MERGE
	barrier_wait(&my_barrier);

	for(int l = 1; l <= (1 << 0); l <<= 1) {
		num_merged_chunks = l * (BLOCK_SIZE / CACHE_SIZE);

		if((tid & ((1<<l)-1)) == 0) {
			for(int i = ((tid+l) * BLOCK_SIZE) - CACHE_SIZE; i >= tid * BLOCK_SIZE; i -= CACHE_SIZE) {
				mram_read(&mem[i], &cache[cache_type][tid][0], sizeof(uint32_t) * CACHE_SIZE);
				merge(tid, cache[cache_type][tid], cache[cache_type ^ 1][tid], &mem[i + CACHE_SIZE], &mem[i], num_merged_chunks);
				num_merged_chunks++;
			}
		}
	}
#endif
	/*** End perf count ***/
	// if (tid == 0)
	// {
	// print results
	// perfcounter_t end_time = perfcounter_get();
	// execution_cycles = end_time;
	// execution_time = ((double)end_time) / CLOCKS_PER_SEC;
	// printf("Execution Time: %lu\n", end_time);
	// }
	/************************/
	return 0;
}
