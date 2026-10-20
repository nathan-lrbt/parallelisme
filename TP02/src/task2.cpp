#include <iostream>
#include <omp.h>
#include <vector>

using namespace std;

void para_copy(vector<float> &from, vector<float> &to) {
#pragma omp parallel for
  for (size_t i = 0; i < from.size(); i++) {
    to[i] = from[i];
  }
}

int main() {
  vector<float> from(10, 5.);
  vector<float> to(10, 0.);

  cout << "Before copy : " << endl;
  cout << "from = " << from[1] << endl;
  cout << "to = " << to[1] << endl;
  para_copy(from, to);
  cout << "After copy : " << endl;
  cout << "from = " << from[1] << endl;
  cout << "to = " << to[1] << endl;
}
