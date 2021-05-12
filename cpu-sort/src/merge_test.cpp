
#include <iostream>  // std::cout
#include <algorithm> // std::sort
#include <vector>    // std::vector
#include <chrono>
#include <execution>

int main(int argc, char *argv[])
{
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;

    uint32_t size = 1024;

    if (argc == 2)
    {
        size = 1 << atoi(argv[1]);
    }

    uint32_t *arr1 = new uint32_t[size];
    uint32_t *arr2 = new uint32_t[size];
    uint32_t *result = new uint32_t[2 * size];

    srand(time(0));

    for (int i = 0; i < size; i++)
    {
        arr1[i] = rand() % 100;
        arr2[i] = rand() % 100;
    }

    std::sort(std::execution::par, arr1, arr1 + size);
    std::sort(std::execution::par, arr2, arr2 + size);

    auto t1 = high_resolution_clock::now();
    std::merge(std::execution::par, arr1, arr1 + size, arr2, arr2 + size, result);
    auto t2 = high_resolution_clock::now();

    /* Getting number of milliseconds as a double. */
    duration<double, std::milli> ms_double = t2 - t1;

    std::cout << "Size: " << size << ", Merge Par Execution Time: " << ms_double.count() / 1000 << std::endl;

    auto t3 = high_resolution_clock::now();
    std::merge(arr1, arr1 + size, arr2, arr2 + size, result);
    auto t4 = high_resolution_clock::now();

    /* Getting number of milliseconds as a double. */
    ms_double = t4 - t3;

    std::cout << "Size: " << size << ", Merge Seq Execution Time: " << ms_double.count() / 1000 << std::endl;

    // --------------------------------------------------------------------------
    for (int i = 0; i < 2 * size; i++)
    {
        result[i] = rand() % 100;
    }

    std::sort(std::execution::par, result, result + size);
    std::sort(std::execution::par, result + size, result + 2 * size);

    auto t5 = high_resolution_clock::now();
    std::inplace_merge(std::execution::par, result, result + size, result + 2 * size);
    auto t6 = high_resolution_clock::now();

    /* Getting number of milliseconds as a double. */
    ms_double = t6 - t5;

    std::cout << "Size: " << size << ", Merge Par Execution Time: " << ms_double.count() / 1000 << std::endl;
}