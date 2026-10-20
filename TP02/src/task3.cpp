#include <iostream>
#include <vector>

using namespace std;

void side_effect_function(float &value, size_t index) {
  value = value + index * 0.1f;
}

int main() {
  vector<float> data(10, 0.);
  float seed = 7.f;
  // TODO
  cout << "==== SHARED ====" << endl;
#pragma omp parallel for shared(seed)
  for (size_t i = 0; i < data.size(); ++i) {
    side_effect_function(seed, i + 1);
    data[i] = seed;
  }
  for (auto v : data) {
    std::cout << v << " ";
  }
  std::cout << std::endl;
  std::cout << seed << std::endl;

  seed = 7.f;
  cout << "==== PRIVATE ====" << endl;
#pragma omp parallel for private(seed)
  for (size_t i = 0; i < data.size(); ++i) {
    side_effect_function(seed, i + 1);
    data[i] = seed;
  }
  for (auto v : data) {
    std::cout << v << " ";
  }
  std::cout << std::endl;
  std::cout << seed << std::endl;

  seed = 7.f;
  cout << "== FIRSTPRIVATE ==" << endl;
#pragma omp parallel for firstprivate(seed)
  for (size_t i = 0; i < data.size(); ++i) {
    side_effect_function(seed, i + 1);
    data[i] = seed;
  }
  for (auto v : data) {
    std::cout << v << " ";
  }
  std::cout << std::endl;
  std::cout << seed << std::endl;
  return 0;
}
