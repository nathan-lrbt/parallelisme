#include <iostream>
#include <omp.h>

using namespace std;

int main() {
  // omp_set_num_threads(4);
#pragma omp parallel
  {
    cout << "Hello from " << omp_get_thread_num() << endl;
  }
}
