// sort algorithm example
#include <iostream>  // std::cout
#include <algorithm> // std::sort
#include <vector>    // std::vector
#include <chrono>
#include <execution>

bool myfunction(int i, int j) { return (i < j); }

struct myclass
{
  bool operator()(int i, int j) { return (i < j); }
} myobject;

int main(int argc, char *argv[])
{

  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;
  using std::chrono::milliseconds;

  uint32_t arr_size = 1024;

  if (argc == 2)
  {
    arr_size = atoi(argv[1]);
  }

  uint32_t *arr = new uint32_t[arr_size];

  for (int i = 0; i < arr_size; i++)
    arr[i] = rand() % 100; //Generate number between 0 to 99

  auto t1 = high_resolution_clock::now();
  std::sort(std::execution::par_unseq, arr, arr + arr_size);
  auto t2 = high_resolution_clock::now();

  /* Getting number of milliseconds as a double. */
  duration<double, std::milli> ms_double = t2 - t1;

  std::cout << "Size: " << arr_size << ", Execution Time: " << ms_double.count() / 1000 << std::endl;
}