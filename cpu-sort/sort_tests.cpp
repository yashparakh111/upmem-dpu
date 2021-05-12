// sort algorithm example
#include <iostream>     // std::cout
#include <algorithm>    // std::sort
#include <vector>       // std::vector
#include <chrono>

bool myfunction (int i,int j) { return (i<j); }

struct myclass {
  bool operator() (int i,int j) { return (i<j);}
} myobject;

int main(int argc, char* argv[])
{

    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;
    
    uint32_t arr_size = 1024;

    if(argc == 2) {
        arr_size = atoi(argv[1]);
    }
    
    std::vector<uint32_t> myvector;

    for(int i=0;i<arr_size;i++)
      myvector.push_back(rand()%100);  //Generate number between 0 to 99
  
    auto t1 = high_resolution_clock::now(); 
    std::sort (myvector.begin(), myvector.end());
    auto t2 = high_resolution_clock::now();

    /* Getting number of milliseconds as a double. */
    duration<double, std::milli> ms_double = t2 - t1;

    std::cout << "Size: " << arr_size << ", Execution Time: " << 1000*ms_double.count() << std::endl;
   
}