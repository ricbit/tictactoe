#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <ctgmath>
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
  cout << "num symmetries " << data.symmetries_size() << "\n";
  vector<int> search_tree(data.board_size);
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine generator(seed);
  int max_plays = 1000;
  vector<int> win_counts(3);
  cout << "winning lines " << data.line_size << "\n";
  for (int i = 0; i < max_plays; ++i) {
    State state(data);
    GameEngine b(generator, state,
        ForcingMove(state) >>
        ForcingStrategy(state, data) >>
        BiasedRandom(state, generator));
        //ForcingMove(state) >> BiasedRandom(state, generator));
        //ForcingMove(state) >> HeatMap(state, data, generator));
    int level = 0;
    Mark winner = b.play(Mark::X, [&](const auto& open_positions) {
      search_tree[level++] += open_positions.count();
    }, [](const auto& x, auto y){});
    win_counts[static_cast<int>(winner)]++;
  }
  double total = 0.0;
  for (int i = 0; i < static_cast<int>(search_tree.size()); ++i) {
    double level = static_cast<double>(search_tree[i]) / max_plays;
    cout << "level " << i << " : " << level << "\n";
    if (level > 0.0) {
      double log_level = log10(level < 1.0 ? 1.0 : level);
      total += log_level;
    }
  }
  cout << "\ntotal : 10 ^ " << total << "\n";
  cout << "X wins : " << win_counts[static_cast<int>(Mark::X)] << "\n";
  cout << "O wins : " << win_counts[static_cast<int>(Mark::O)] << "\n";
  cout << "draws  : " << win_counts[static_cast<int>(Mark::empty)] << "\n";
  return 0;
}

