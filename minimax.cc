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

int main(int argc, char **argv) {
  constexpr int N = 4;
  constexpr int D = 3;
  BoardData<N, D> data;
  State state(data);
  auto minimax = MiniMax<N, D, 1000000>(state, data);
  auto result = minimax.play(state, Mark::X);
  cout << *result << "\n";
  if (argc >= 2) {
    minimax.get_solution().dump(data, argv[1]);
  }
  return 0;
}
