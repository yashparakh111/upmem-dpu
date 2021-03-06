/*
 *  Performs sorting using DPUs
 */

#include <stdio.h>

#include <dpu.h>
#include <dpu_log.h>
#include <time.h>

#include "config.h"

#ifndef DPU_BINARY
#define DPU_BINARY "./build/dpu_sort"
#endif

void print_arr_2(char *name, uint32_t *arr, int size)
{
    printf("%s: \n", name);
    for (int i = 0; i < size; i++)
    {
        printf("%02u ", arr[i]);
    }
    printf("\n\n");
}

// sorts the in_arr in BUFFER_SIZE chunks
void sort_dpu(uint32_t size, uint32_t *in_arr, uint32_t *out_arr)
{
    struct dpu_set_t set, dpu;
    uint32_t each_dpu, num_dpus, num_dpu_needed, dpu_alloc_ct, dpu_loop_cnt;

    num_dpu_needed = (size + (BUFFER_SIZE - 1)) / BUFFER_SIZE;
    dpu_alloc_ct = (num_dpu_needed > MAX_DPU_AVAIL) ? MAX_DPU_AVAIL : num_dpu_needed;
    DPU_ASSERT(dpu_alloc(dpu_alloc_ct, "backend=simulator", &set));
    DPU_ASSERT(dpu_get_nr_dpus(set, &num_dpus));

    dpu_loop_cnt = (num_dpu_needed + (num_dpus - 1)) / num_dpus;

    //printf("Number of DPUs needed: %d\n", num_dpu_needed);
    //printf("Using %u dpu(s)\n", num_dpus);
    //printf("Using %u dpu loop(s)\n", dpu_loop_cnt);

    // load the binary into the dpu
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    // load data into the dpu
    // using xfer lets us maximize transfer bandwidth
    for (int dpu_loop = 0; dpu_loop < dpu_loop_cnt; dpu_loop++)
    {
        //printf("loop: %d\n", dpu_loop);
        //may not need all dpus for last loo..8}
        uint32_t num_dpu_needed_iter = ((dpu_loop + 1) == dpu_loop_cnt) ? (dpu_alloc_ct - (num_dpu_needed % num_dpus)) : num_dpus;
        //printf("Need %d DPUs for cycle %d\n", num_dpu_needed_iter, dpu_loop);

        DPU_FOREACH(set, dpu, each_dpu)
        {
            //if (each_dpu < num_dpu_needed_iter)
            DPU_ASSERT(dpu_prepare_xfer(dpu, &in_arr[(dpu_loop * num_dpus + each_dpu) * BUFFER_SIZE]));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "mem", 0, sizeof(uint32_t) * BUFFER_SIZE, DPU_XFER_ASYNC));

        // start executing program
        DPU_ASSERT(dpu_launch(set, DPU_ASYNCHRONOUS));

        // loops through each DPU in SET
        DPU_FOREACH(set, dpu, each_dpu)
        {
            // get data from the dpu
            //if (each_dpu < num_dpu_needed_iter)
            DPU_ASSERT(dpu_prepare_xfer(dpu, &out_arr[(dpu_loop * num_dpus + each_dpu) * BUFFER_SIZE]));
            // DPU_ASSERT(dpu_log_read(dpu, stdout));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, "mem", 0, sizeof(uint32_t) * BUFFER_SIZE, DPU_XFER_ASYNC));

        // sync barrier for all dpus

        DPU_ASSERT(dpu_sync(set));
    }
    // release the allocated dpus
    DPU_ASSERT(dpu_free(set));
}

void merge_blocks(int i, int j, uint32_t *a, uint32_t *aux)
{

    int size = j - i + 1;
    if (size <= (BUFFER_SIZE / NR_TASKLETS))
    {
        //printf("Returning with size: %d, buf_size: %d", size, BUFFER_SIZE);
        return; // the subsection is empty or a single element
    }
    int mid = (i + j) / 2;

    // left sub-array is a[i .. mid]
    // right sub-array is a[mid + 1 .. j]

    merge_blocks(i, mid, a, aux);     // sort the left sub-array recursively
    merge_blocks(mid + 1, j, a, aux); // sort the right sub-array recursively

    int pointer_left = i;        // pointer_left points to the beginning of the left sub-array
    int pointer_right = mid + 1; // pointer_right points to the beginning of the right sub-array
    int k;                       // k is the loop counter

    //printf("I: %d, J: %d, MID: %d, size: %d", i, j, mid, j - i + 1);

    // we loop from i to j to fill each el\nement of the fjnalimerged array
    for (k = i; k <= j; k++)
    {
        if (pointer_left == mid + 1)
        { // left pointer has reached the limit
            aux[k] = a[pointer_right];
            pointer_right++;
        }
        else if (pointer_right == j + 1)
        { // right pointer has reached the limit
            aux[k] = a[pointer_left];
            pointer_left++;
        }
        else if (a[pointer_left] < a[pointer_right])
        { // pointer left points to smaller element
            aux[k] = a[pointer_left];
            pointer_left++;
        }
        else
        { // pointer right points to smaller element
            aux[k] = a[pointer_right];
            pointer_right++;
        }
    }

    for (k = i; k <= j; k++)
    { // copy the elements from aux[] to a[]
        a[k] = aux[k];
    }
}

void sort_pim(uint32_t size, uint32_t *in_arr, uint32_t *out_arr)
{
    clock_t start, mid, end;
    double dpu_exec_time, cpu_exec_time;

    start = clock();

    sort_dpu(size, in_arr, out_arr);

    mid = clock();

    merge_blocks(0, size - 1, out_arr, in_arr);

    end = clock();
    
    dpu_exec_time = (((double)(mid - start)) / CLOCKS_PER_SEC);
    cpu_exec_time = (((double)(end - mid)) / CLOCKS_PER_SEC);
    printf("%f   %f   %f ", dpu_exec_time, cpu_exec_time, dpu_exec_time + cpu_exec_time);
}