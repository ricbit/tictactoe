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
#include "strategies.hh"

int main() {
  BoardData<5, 3> data;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine generator(seed);
  State state(data);
  auto s =
      ForcingMove(state) >>
      ForcingStrategy(state, data) >>
      HeatMap(state, data, generator, 100, true);
  //auto s = HeatMap(state, data, generator);
  GameEngine b(generator, state, s);
  int current = 0;
  b.play(Mark::X, [&](auto obs) {
    cout << "\x1b[0m\n\nlevel " << current++ << "\n";
  }, [&](const auto& state, auto pos) {
    state.print_last_position(*pos);
  });
  cout << "\nfinal\n";
  state.print_winner();
  return 0;
}
