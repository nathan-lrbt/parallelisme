#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
  auto v = vector{1, 2, 3};
  auto response = "";

  if (v.size() % 2 == 0) {
    response = "EVEN";
  } else {
    response = "ODD";
  }
  cout << "vector v contains an " << response << " number of elements" << endl;
}
