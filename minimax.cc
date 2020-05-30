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
#include "tictactoe.hh"

int main() {
  BoardData<4, 2> data;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine generator(seed);
  State state(data);
  auto s = MiniMax(state, data, generator);
  auto result = s.play(state, Mark::X);
  if (*result == Mark::X) {
    cout << "X wins\n";
  } else if (*result == Mark::O) {
    cout << "O wins\n";
  } else {
    cout << "Draw\n";
  }
  return 0;
}
