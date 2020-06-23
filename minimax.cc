#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <set>
#include <queue>
#include <cassert>
#include <bitset>
#include <execution>
#include <list>
#include "minimax.hh"

int main() {
  constexpr int N = 5;
  constexpr int D = 3;
  BoardData<N, D> data;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine generator(seed);
  State state(data);
  auto minimax = MiniMax<N, D, 1000000>(state, data, generator);
  auto result = minimax.play(state, Mark::X);
  cout << *result << "\n";
  minimax.get_solution().dump(data, "solution.txt");
  return 0;
}
