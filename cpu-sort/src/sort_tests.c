#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>

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

void cpu_sort2(uint32_t size, uint32_t *in_arr){
    qsort(in_arr, size, sizeof(uint32_t), comp);
}

void gen_rand_arr(uint32_t *arr, int size)
{
  for (int i = 0; i < size; i++)
  {
    arr[i] = rand() % 100;
  }
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

int main(int argc, char *argv[]) {
    uint32_t arr_size = 1024;
    uint32_t *in_arr;

    if(argc == 2) {
        arr_size = atoi(argv[1]);
    }

    prepare_data(1, arr_size, &in_arr);
    gen_rand_arr(in_arr, arr_size);

    clock_t start, end;
    double exec_time;

    start = clock();
    cpu_sort2(arr_size, in_arr);
    end = clock();

    exec_time = (((double)(end - start)) / CLOCKS_PER_SEC);
    printf("Size: %d, Execution Time: %f\n", arr_size, exec_time);

}