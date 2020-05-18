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
  Geometry<5, 3> geom;
  Symmetry sym(geom);
  SymmeTrie trie(sym);
  cout << "num symmetries " << sym.symmetries().size() << "\n";
  vector<int> search_tree(geom.lines_through_position().size());
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine generator(seed);
  int max_plays = 10000;
  vector<int> win_counts(3);
  //geom.print_points();
  cout << "winning lines " << geom.line_size << "\n";
  for (int i = 0; i < max_plays; ++i) {
    State state(geom, sym, trie);
    GameEngine b(geom, sym, trie, generator, state);
    int level = 0;
    Mark winner = b.play(Mark::X, [&](const auto& open_positions) {    
      search_tree[level++] += open_positions.count();
    });
    win_counts[static_cast<int>(winner)]++;
  }
  double total = 0.0;
  for (int i = 0; i < static_cast<int>(search_tree.size()); ++i) {
    double level = static_cast<double>(search_tree[i]) / max_plays;
    cout << "level " << i << " : " << level << "\n";
    if (level > 0.0) {
      double log_level = log10(level < 1.0 ? 1.0 : level);
      total += log_level;
      //cout << log_level << "\n";
    }
  }
  cout << "\ntotal : 10 ^ " << total << "\n";
  cout << "X wins : " << win_counts[static_cast<int>(Mark::X)] << "\n";
  cout << "O wins : " << win_counts[static_cast<int>(Mark::O)] << "\n";
  cout << "draws  : " << win_counts[static_cast<int>(Mark::empty)] << "\n";
  return 0;
}

