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
  auto minimax = MiniMax(state, data, generator);
  auto result = minimax.play(state, Mark::X);
  if (*result == BoardValue::X_WIN) {
    cout << "X wins\n";
  } else if (*result == BoardValue::O_WIN) {
    cout << "O wins\n";
  } else {
    cout << "Draw\n";
  }
  minimax.get_solution().dump(data, "solution.txt");
  return 0;
}
