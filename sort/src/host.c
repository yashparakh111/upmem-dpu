#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "sort.h"

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
  assert(BLOCK_SIZE > CACHE_SIZE);
  assert(arr_size >= BUFFER_SIZE);

  // alloc array space
  prepare_data(3, arr_size, &in_arr, &out_arr, &out_arr_golden);

  // generate input array
  gen_rand_arr(in_arr, arr_size);

  // perform cpu+dpu sort
  sort(arr_size, in_arr, out_arr);

  // generate golden data
  memcpy(out_arr_golden, in_arr, sizeof(uint32_t) * arr_size);
  qsort(out_arr_golden, arr_size, sizeof(uint32_t), comp);

  // print results
  if (do_arr_print)
  {
    print_arr("In", in_arr, arr_size);
    print_arr("Out DPU", out_arr, arr_size);
    print_arr("Out CPU", out_arr_golden, arr_size);
  }

  // verify results
  num_dpu_needed = (arr_size + (BUFFER_SIZE - 1)) / BUFFER_SIZE;
  if (verify_chunks(out_arr, in_arr, arr_size, arr_size / (num_dpu_needed * NR_TASKLETS)))
    printf("Chunks Sorted!\n");
  else
    printf("Unsorted Chunks\n");

  bool sorted = true;
  for (int i = 0; i < arr_size; i++)
  {
    if (out_arr_golden[i] != out_arr[i])
      sorted = false;
  }
  if (sorted)
    printf("Fully Sorted\n");
  else
    printf("Unsorted\n");

  // free array space
  free_data(3, in_arr, out_arr, out_arr_golden);

  return 0;
}
