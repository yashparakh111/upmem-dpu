#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cassert>

#include "config.h"
#include "sort.hpp"
//#define BUFFER_SIZE 16
//#define BLOCK_SIZE 16
//#define CACHE_SIZE 4
//#define NR_TASKLETS 2

using namespace std;


void print_arr(string s, vector<uint32_t> arr)
{
  cout << s << ": ";
  for (auto i = arr.begin(); i != arr.end(); ++i)
    cout << *i << ' ';
  cout << endl;
}

uint32_t rand_fn()
{
  return rand() % 100;
}

void verify(vector<uint32_t> dpu_arr, vector<uint32_t> golden_arr, vector<uint32_t> cpu_arr, int chunk_size)
{
  vector<uint32_t> golden_chunk;
  vector<uint32_t> golden_chunk_arr;
  int iters = golden_arr.size() / chunk_size;
  bool fail = false;
  for (int i = 0; i < iters; i++)
  {
    auto golden_base = i * chunk_size + cpu_arr.begin();
    auto dpu_base = i * chunk_size + dpu_arr.begin();

    golden_chunk = vector<uint32_t>(golden_base, golden_base + chunk_size);
    sort(golden_chunk.begin(), golden_chunk.end());
    // Debug
    //golden_chunk_arr.insert(golden_chunk_arr.end(), golden_chunk.begin(), golden_chunk.end());

    fail |= (!equal(golden_chunk.begin(), golden_chunk.end(), dpu_base));
  }
  // Debug
  //print_arr("Chunk arr", golden_chunk_arr);
  //merge_blocks(golden_chunk_arr, chunk_size);

  //print_arr("GC arr   ", golden_chunk_arr);
  //print_arr("Out arr golden", dpu_arr);

  if (fail)
    cout << "Chunks_Unsorted ";
  else
    cout << "Chunks_Sorted ";
  // Verify entire array
  if (golden_arr == dpu_arr)
    cout << "Success ";
  else
    cout << "Failure ";
}

int main(int argc, char *argv[])
{
  uint32_t min_arr_size, max_arr_size;
  bool do_print = 0;

  srand(uint32_t(time(nullptr)));

  if (argc < 4)
  {
    cout << "Usage: ./host <arr_size> [<print>]";
  }
  else
  {
    min_arr_size = (1 << stoi(argv[1]));
    max_arr_size = (1 << stoi(argv[2]));
    do_print = (bool)stoi(argv[3]);
  }

  allocate_dpus(max_arr_size);
  
  for(uint32_t arr_size = min_arr_size; arr_size <= max_arr_size; arr_size <<= 1){
    assert(arr_size >= BUFFER_SIZE);
    assert(MAX_BUFFER_SIZE >= BUFFER_SIZE);
    assert(BUFFER_SIZE >= BLOCK_SIZE);
    assert(BLOCK_SIZE >= CACHE_SIZE);

    std::vector<uint32_t> in_arr(arr_size);
    std::vector<uint32_t> out_arr(arr_size);

    generate(in_arr.begin(), in_arr.end(), rand_fn);
    std::vector<uint32_t> out_arr_golden = in_arr;

    if (do_print)
    {
      print_arr("In", in_arr);
    }

    printf("% 10d % 8d % 4d ", arr_size, BUFFER_SIZE, CACHE_SIZE);

    sort_pim(in_arr, out_arr);

    sort(out_arr_golden.begin(), out_arr_golden.end());

    if (do_print)
    {
      print_arr("Out CPU", out_arr_golden);
      print_arr("Out DPU", out_arr);
    }

    verify(out_arr, out_arr_golden, in_arr, BLOCK_SIZE);

    cout << endl;
  }

  free_dpus();


  return 0;
}