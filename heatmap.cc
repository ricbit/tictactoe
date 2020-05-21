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
  BoardData<5, 3> data;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine generator(seed);
  State state(data);
  auto s = HeatMap<5, 3>(state, data, generator);
  GameEngine b(generator, state, s);
  int current = 0;
  b.play(Mark::X, [&](auto obs) {
    cout << "\n\nlevel " << current++ << "\n"; 
  });
  return 0;
}
