#include <iostream>
#include <omp.h>
#include <vector>

using namespace std;

int main() {
  vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  int sum = 0;
#pragma omp parallel for reduction(+ : sum)
  for (size_t i = 0; i < data.size(); ++i) {
    sum += data[i];
  }
  cout << "sum = " << sum << endl;

  int max;

  max = data[0];
#pragma omp parallel for reduction(max : max)
  for (size_t i = 0; i < data.size(); i++) {
    if (data[i] > max) {
      max = data[i];
    }
  }

  cout << "max = " << max << endl;
}
