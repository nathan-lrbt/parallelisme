#define _USE_MATH_DEFINES
#include <chrono>
#include <cmath>
#include <iostream>
#include <omp.h>

using namespace std;

double f(double x) { return 1 / (1 + x * x); }

double riemann_sum(double a, double b, int n) {
  double dx = (b - a) / n;
  double sum = 0.;
  double x_star;
  for (size_t i = 0; i < n; i++) {
    x_star = a + (i + 0.5f) * dx;
    sum += f(x_star) * dx;
  }
  return sum;
}

double riemann_sum_omp(double a, double b, size_t n) {
  double dx = (b - a) / n;
  double sum = 0.;
  double x_star;
#pragma omp parallel for reduction(+ : sum) private(x_star)
  for (size_t i = 0; i < n; i++) {
    x_star = a + (i + 0.5f) * dx;
    sum += f(x_star) * dx;
  }
  return sum;
}

double arctan(double x) { return riemann_sum(0, 1, 100000000); }
double arctan_omp(double x) { return riemann_sum_omp(0, 1, 100000000); }

int main() {
  cout << "pi / 4 = " << M_PI / 4 << endl;
  auto start = chrono::high_resolution_clock::now();
  cout << "arctan(1) = " << arctan(1.) << endl;
  auto end = chrono::high_resolution_clock::now();
  cout << "Elapsed time: "
       << chrono::duration_cast<chrono::milliseconds>(end - start).count()
       << " ms" << endl;

  start = chrono::high_resolution_clock::now();
  cout << "arctan_omp(1) = " << arctan_omp(1.) << endl;
  end = chrono::high_resolution_clock::now();
  cout << "Elapsed time: "
       << chrono::duration_cast<chrono::milliseconds>(end - start).count()
       << " ms" << endl;
}
