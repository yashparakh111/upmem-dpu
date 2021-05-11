#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "sort.h"

extern uint32_t execution_cycles;
extern double execution_time;

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
    arr[i] = rand() % 100;
  }
}

void print_arr(char *name, uint32_t *arr, int size)
{
  printf("%s: \n", name);
  for (int i = 0; i < size; i++)
  {
    printf("%02u ", arr[i]);
  }
  printf("\n\n");
}

void verify(uint32_t *dpu_arr, uint32_t *golden_arr, uint32_t *cpu_arr, int size, int chunk_size)
{
  uint32_t golden_chunk[chunk_size];
  uint32_t iters = size / chunk_size;
  bool pass = true;
  bool sorted = true;

  // verify at chunk granularity
  for (int i = 0; i < iters; i++)
  {
    uint32_t base = i * chunk_size;
    memcpy(golden_chunk, &cpu_arr[base], sizeof(uint32_t) * chunk_size);
    qsort(golden_chunk, chunk_size, sizeof(uint32_t), comp);
    // if (i == 2)
    // {
    //   print_arr("actual 2", dpu_arr + i * chunk_size, chunk_size);
    //   print_arr("golden 2", golden_chunk, chunk_size);
    // }
    bool chunk_sorted = true;
    for (int j = 0; j < chunk_size; j++)
    {
      if (golden_chunk[j] != dpu_arr[base + j])
      {
        // printf("Error at block: %d, idx: %d. Expected %d, got %d\n", i, j,
        //        golden_chunk[j], dpu_arr[base + j]);
        chunk_sorted = false;
        pass = false;
      }
    }
    // printf("Chunk %d: %d\n", i, chunk_sorted);
  }
  if (pass)
    printf("Chunks_Sorted ");
  else
    printf("Chunks_Unsorted ");

  // verify entire arry
  for (int i = 0; i < size; i++)
  {
    if (golden_arr[i] != dpu_arr[i])
      sorted = false;
  }
  if (sorted)
    printf("Success ");
  else
    printf("Failure ");
}

void prepare_data(uint32_t nargs, uint32_t size, ...)
{
  va_list ap;
  uint32_t **arr;

  // allocate space per arg
  va_start(ap, size);
  for (int i = 0; i < nargs; i++)
  {
    arr = va_arg(ap, uint32_t **);
    *arr = (uint32_t *)malloc(sizeof(uint32_t) * size);
  }
  va_end(ap);
}

void free_data(uint32_t nargs, ...)
{
  va_list ap;
  uint32_t *arr;

  // allocate space per arg
  va_start(ap, nargs);
  for (int i = 0; i < nargs; i++)
  {
    arr = va_arg(ap, uint32_t *);
    free(arr);
  }
  va_end(ap);
}

int main(int argc, char *argv[])
{
  uint32_t *in_arr, *out_arr, *out_arr_golden;
  uint32_t arr_size;
  bool do_arr_print;

  // get array size
  if (argc == 1)
  {
    printf("Usage: ./host <arr_size> [<print results>]");
    exit(-1);
  }
  else
  {
    arr_size = atoi(argv[1]);
    if (argc > 2)
      do_arr_print = (bool)atoi(argv[2]);
    else
      do_arr_print = false;
  }

  // safety checks
  assert(arr_size >= BUFFER_SIZE);
  assert(BUFFER_SIZE >= BLOCK_SIZE);
  assert(BLOCK_SIZE >= CACHE_SIZE);

  // alloc array space
  prepare_data(3, arr_size, &in_arr, &out_arr, &out_arr_golden);

  // generate input array
  gen_rand_arr(in_arr, arr_size);

  // print input array
  if (do_arr_print)
  {
    print_arr("In", in_arr, arr_size);
  }

  // perform cpu+dpu sort
  sort(arr_size, in_arr, out_arr);

  // generate golden data
  memcpy(out_arr_golden, in_arr, sizeof(uint32_t) * arr_size);
  qsort(out_arr_golden, arr_size, sizeof(uint32_t), comp);

  // print results
  if (do_arr_print)
  {
    print_arr("Out DPU", out_arr, arr_size);
    print_arr("Out CPU", out_arr_golden, arr_size);
  }

  // verify results
  // num_dpu_needed = (arr_size + (BUFFER_SIZE - 1)) / BUFFER_SIZE;
  verify(out_arr, out_arr_golden, in_arr, arr_size, BUFFER_SIZE / NR_TASKLETS);

  // free array space
  // free_data(3, in_arr, out_arr, out_arr_golden);
  // free(in_arr);
  // free(out_arr);
  // free(out_arr_golden);

  printf("%d %d %d %f\n", arr_size, CACHE_SIZE, execution_cycles, execution_time);

  return 0;
}
