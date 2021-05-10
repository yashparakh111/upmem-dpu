#include <assert.h>
#include <dpu.h>
#include <dpu_log.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"

#ifndef DPU_BINARY
#define DPU_BINARY "./build/radix_sort"
#endif

int comp(const void *elem1, const void *elem2)
{
  int f = *((int *)elem1);
  int s = *((int *)elem2);
  if (f > s)
    return 1;
  if (f < s)
    return -1;
  return 0;
}

void gen_rand_arr(uint32_t *arr, int size)
{
  for (int i = 0; i < size; i++)
  {
    arr[i] = rand() % 50;
  }
}

void print_arr(char *name, uint32_t *arr, int size)
{
  printf("%s: ", name);
  for (int i = 0; i < size; i++)
  {
    printf("%02u ", arr[i]);
  }
  printf("\n\n");
}

bool verify_chunks(uint32_t *dpu_arr, uint32_t *cpu_arr, int size, int chunk_size)
{
  uint32_t golden_chunk[chunk_size];
  uint32_t iters = size / chunk_size;
  bool pass = true;
  for (int i = 0; i < iters; i++)
  {
    uint32_t base = i * chunk_size;
    memcpy(golden_chunk, &cpu_arr[base], sizeof(uint32_t) * chunk_size);
    qsort(golden_chunk, chunk_size, sizeof(uint32_t), comp);

    for (int j = 0; j < chunk_size; j++)
    {
      if (golden_chunk[j] != dpu_arr[base + j])
      {
        printf("Error at block: %d, idx: %d. Expected %d, got %d\n", i, j,
               golden_chunk[j], dpu_arr[base + j]);
        pass = false;
      }
    }
  }
  return pass;
}

void launch_dpu(uint32_t *in_arr, uint32_t *out_arr)
{
  struct dpu_set_t set, dpu;
  uint32_t each_dpu, num_dpus;

  // allocate 1 DPU (max available is 1 on sim)
  DPU_ASSERT(dpu_alloc(1, "backend=simulator", &set));
  DPU_ASSERT(dpu_get_nr_dpus(set, &num_dpus));

  printf("Using %u dpu(s)\n", num_dpus);

  // load the binary into the dpu
  DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

  // load data into the dpu
  // using xfer lets us maximize transfer bandwidth
  DPU_FOREACH(set, dpu, each_dpu)
  {
    DPU_ASSERT(dpu_prepare_xfer(dpu, &in_arr[each_dpu * BUFFER_SIZE]));
  }
  DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "mem", 0, sizeof(uint32_t) * BUFFER_SIZE, DPU_XFER_ASYNC));

  // start executing program
  DPU_ASSERT(dpu_launch(set, DPU_ASYNCHRONOUS));

  // loops through each DPU in SET
  DPU_FOREACH(set, dpu, each_dpu)
  {
    // get data from the dpu
    DPU_ASSERT(dpu_prepare_xfer(dpu, &out_arr[each_dpu * BUFFER_SIZE]));
    // DPU_ASSERT(dpu_log_read(dpu, stdout));
  }
  DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, "mem", 0, sizeof(uint32_t) * BUFFER_SIZE, DPU_XFER_ASYNC));

  // synch barrier for all dpus
  DPU_ASSERT(dpu_sync(set));

  // release the allocated dpus
  DPU_ASSERT(dpu_free(set));
}

int main(void)
{
  uint32_t in_arr[ARRAY_SIZE];
  uint32_t out_arr[ARRAY_SIZE];
  uint32_t out_arr_golden[ARRAY_SIZE];

  assert(BLOCK_SIZE > CACHE_SIZE);
  assert(ARRAY_SIZE >= BUFFER_SIZE);

  // generate input array
  gen_rand_arr(in_arr, ARRAY_SIZE);

  launch_dpu(in_arr, out_arr);
  memcpy(out_arr_golden, in_arr, sizeof(uint32_t) * ARRAY_SIZE);
  qsort(out_arr_golden, ARRAY_SIZE, sizeof(uint32_t), comp);

  print_arr("In", in_arr, ARRAY_SIZE);
  print_arr("Out DPU", out_arr, ARRAY_SIZE);
  print_arr("Out CPU", out_arr_golden, ARRAY_SIZE);
  if (verify_chunks(out_arr, in_arr, ARRAY_SIZE, ARRAY_SIZE / NR_TASKLETS))
    printf("Success!\n");
  else
    printf("Fail!\n");
  return 0;
}
