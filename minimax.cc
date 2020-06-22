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
  constexpr int N = 4;
  constexpr int D = 2;
  BoardData<N, D> data;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine generator(seed);
  State state(data);
  auto minimax = MiniMax<N, D, 10000000>(state, data, generator);
  auto result = minimax.play(state, Mark::X);
  if (*result == BoardValue::X_WIN) {
    cout << "X wins\n";
  } else if (*result == BoardValue::O_WIN) {
    cout << "O wins\n";
  } else if (*result == BoardValue::DRAW) {
    cout << "Draw\n";
  } else {
    cout << "Unknown\n";
  }
  minimax.get_solution().dump(data, "solution.txt");
  return 0;
}
