#include <vector>
#include <algorithm>
#include <dpu>
#include <ctime>
#include <iostream>

#include "config.h"

#ifndef DPU_BINARY
#define DPU_BINARY "./build/dpu_sort"
#endif


using namespace dpu;

// sorts the in_arr in BUFFER_SIZE chunks
void sort_dpu(uint32_t size, uint32_t *in_arr, uint32_t *out_arr)
{
    struct dpu_set_t set, dpu;
    uint32_t each_dpu, num_dpus, num_dpu_needed, dpu_alloc_ct;
    int dpu_loop_cnt;

    auto system = DpuSet::allocate(dpu_alloc_ct, "backend=simulator");
    auto dpus = system.dpus();

    num_dpus = dpus.size();
    dpu_loop_cnt = (num_dpu_needed + (num_dpus - 1)) / num_dpus;
    num_dpu_needed = (size + (BUFFER_SIZE - 1)) / BUFFER_SIZE;
    dpu_alloc_ct = (num_dpu_needed > MAX_DPU_AVAIL) ? MAX_DPU_AVAIL : num_dpu_needed;

    printf("Number of DPUs needed: %d\n", num_dpu_needed);
    printf("Using %u dpu(s)\n", num_dpus);
    printf("Using %u dpu loop(s)\n", dpu_loop_cnt);

    // load the binary into the dpu
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
    }

    // automatic release of allocated dpus by destructor
}

// sorts the in_arr in BUFFER_SIZE chunks
void sort_dpu_c_api(uint32_t size, uint32_t *in_arr, uint32_t *out_arr)
{
    struct dpu_set_t set, dpu;
    uint32_t each_dpu, num_dpus, num_dpu_needed, dpu_alloc_ct, dpu_loop_cnt;
    
    num_dpu_needed = (size + (BUFFER_SIZE - 1)) / BUFFER_SIZE;
    dpu_alloc_ct = (num_dpu_needed > MAX_DPU_AVAIL) ? MAX_DPU_AVAIL : num_dpu_needed;

    // printf("Max: %d, DPU_alloc: %d, num_dpu_needed: %d\n", MAX_DPU_AVAIL, dpu_alloc_ct, num_dpu_needed);
    DPU_ASSERT(dpu_alloc(dpu_alloc_ct, "backend=simulator", &set));
    DPU_ASSERT(dpu_get_nr_dpus(set, &num_dpus));

    dpu_loop_cnt = (num_dpu_needed + (num_dpus - 1)) / num_dpus;

    // printf("Number of DPUs needed: %d\n", num_dpu_needed);
    // printf("Using %u dpu(s)\n", num_dpus);
    // printf("Using %u dpu loop(s)\n", dpu_loop_cnt);

    // load the binary into the dpu
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    // load data into the dpu
    // using xfer lets us maximize transfer bandwidth
    for (int dpu_loop = 0; dpu_loop < dpu_loop_cnt; dpu_loop++)
    {
        //printf("loop: %d\n", dpu_loop);
        //may not need all dpus for last loop -- TODO
        uint32_t num_dpu_needed_iter = ((dpu_loop + 1) == dpu_loop_cnt) ? (dpu_alloc_ct - (num_dpu_needed % num_dpus)) : num_dpus;
        //printf("Need %d DPUs for cycle %d\n", num_dpu_needed_iter, dpu_loop);

        DPU_FOREACH(set, dpu, each_dpu)
        {
            if (each_dpu < num_dpu_needed_iter)
                DPU_ASSERT(dpu_prepare_xfer(dpu, &in_arr[(dpu_loop * num_dpus + each_dpu) * BUFFER_SIZE]));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "mem", 0, sizeof(uint32_t) * BUFFER_SIZE, DPU_XFER_ASYNC));

        // start executing program
        DPU_ASSERT(dpu_launch(set, DPU_ASYNCHRONOUS));

        // loops through each DPU in SET
        DPU_FOREACH(set, dpu, each_dpu)
        {
            // get data from the dpu
            if (each_dpu < num_dpu_needed_iter)
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

void merge_blocks(std::vector<uint32_t> &arr, int block_size)
{
  int blocks = arr.size() / block_size;
  for (int i = blocks; i > 1; i >>= 1)
  {
    int merge_block_size = arr.size() / i;
    //cout << "Iter: " << i << "  MergeBS: " << merge_block_size <<  endl;
    for (int j = 0; j < (i>>1); j++)
    {
      std::vector<uint32_t>::iterator left_merge = arr.begin() + (j << 1) * merge_block_size;
      std::vector<uint32_t>::iterator mid_merge = left_merge + merge_block_size;
      std::vector<uint32_t>::iterator right_merge = mid_merge + merge_block_size;
      std::inplace_merge(left_merge, mid_merge, right_merge);
    }
  }
}

void sort_pim(std::vector<uint32_t> in_arr, std::vector<uint32_t> &out_arr)
{
    clock_t start, mid, end;
    double dpu_exec_time, cpu_exec_time;
    start = clock();

    // dpu sort
    sort_dpu_c_api(in_arr.size(), in_arr.data(), out_arr.data());

    mid = clock();

    // cpu sort
    merge_blocks(out_arr, BUFFER_SIZE / NR_TASKLETS);

    end = clock();

    dpu_exec_time = (((double)(mid - start)) / CLOCKS_PER_SEC);
    cpu_exec_time = (((double)(end - mid)) / CLOCKS_PER_SEC);

    std::cout << dpu_exec_time << "   " << cpu_exec_time << "   " << dpu_exec_time + cpu_exec_time << "   ";
}