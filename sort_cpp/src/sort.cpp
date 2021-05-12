/*
 *  Performs sorting using DPUs
 */

#include <stdio.h>

//#include <dpu.h>
//#include <dpu_log.h>

#include <dpu>

#include "config.h"

#ifndef DPU_BINARY
#define DPU_BINARY "./build/radix_sort"
#endif

uint32_t execution_cycles;
double execution_time;

using namespace dpu;

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
    uint32_t each_dpu, num_dpus, num_dpu_needed;

    num_dpu_needed = (size + (BUFFER_SIZE - 1)) / BUFFER_SIZE;
    uint32_t dpu_alloc_ct = (num_dpu_needed > DPU_ALLOCATE_ALL) ? DPU_ALLOCATE_ALL : num_dpu_needed;

    //DPU_ASSERT(dpu_alloc(dpu_alloc_ct, NULL, &set));
    //DPU_ASSERT(dpu_get_nr_dpus(set, &num_dpus));
    auto system = DpuSet::allocate(dpu_alloc_ct, "backend=simulator");
    auto dpus = system.dpus();

    num_dpus = dpus.size();

    int dpu_loop_cnt = (num_dpu_needed + (num_dpus - 1)) / num_dpus;

    printf("Number of DPUs needed: %d\n", num_dpu_needed);
    printf("Using %u dpu(s)\n", num_dpus);
    printf("Using %u dpu loop(s)\n", dpu_loop_cnt);

    // load the binary into the dpu
    //DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));
    for(uint32_t each_dpu = 0; each_dpu < dpus.size(); each_dpu++){
        dpus[each_dpu]->load(DPU_BINARY);
    }

    printf("Here0\n");

        // load data into the dpu
    // using xfer lets us maximize transfer bandwidth
    for (int dpu_loop = 0; dpu_loop < dpu_loop_cnt; dpu_loop++)
    {
    
        std::vector<std::vector<uint32_t>> in_arr_wrp; 
        std::vector<std::vector<uint32_t>> out_arr_wrp(num_dpus, std::vector<uint32_t>(num_dpus, 0));

        for(uint32_t each_dpu = 0; each_dpu < dpus.size(); each_dpu++){
            uint32_t offset = (dpu_loop * num_dpus + each_dpu) * BUFFER_SIZE;
            in_arr_wrp.push_back(std::vector<uint32_t>(in_arr + offset, in_arr + offset + BUFFER_SIZE));
        }

        printf("Here1\n");

        //printf("loop: %d\n", dpu_loop);
        //may not need all dpus for last loop -- TODO
        uint32_t num_dpu_needed_iter = ((dpu_loop + 1) == dpu_loop_cnt) ? (dpu_alloc_ct - (num_dpu_needed % num_dpus)) : num_dpus;
        //printf("Need %d DPUs for cycle %d\n", num_dpu_needed_iter, dpu_loop);

        for(int each_dpu = 0; each_dpu < dpus.size(); each_dpu++){
            dpus[each_dpu]->async().copy("mem", 0, in_arr_wrp, BUFFER_SIZE*sizeof(uint32_t));
            dpus[each_dpu]->async().exec();
            dpus[each_dpu]->async().copy(out_arr_wrp, BUFFER_SIZE*sizeof(uint32_t), "mem", 0);
            dpus[each_dpu]->async().sync();
        }

        printf("Here2\n");
        //system.sync();

        /*DPU_FOREACH(set, dpu, each_dpu)
        {
            // get data from the dpu
            //if (each_dpu < num_dpu_needed_iter)
            //{
            DPU_ASSERT(dpu_copy_from(dpu, "execution_cycles", 0, &execution_cycles, sizeof(uint32_t)));
            DPU_ASSERT(dpu_copy_from(dpu, "execution_time", 0, &execution_time, sizeof(double)));
            // }
        }*/

    }
    // release the allocated dpus
    //DPU_ASSERT(dpu_free(set));
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

    int pointer_left = i;    // pointer_left points to the beginning of the left sub-array
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

void sort(uint32_t size, uint32_t *in_arr, uint32_t *out_arr)
{
    sort_dpu(size, in_arr, out_arr);
    //print_arr_2("Post-dpu sort", out_arr, arr_size);
    merge_blocks(0, size - 1, out_arr, in_arr);
}
