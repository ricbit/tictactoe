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

struct DebugConfig {
  constexpr static NodeCount max_visited = 10'000'000_nc;
  constexpr static NodeCount max_created = 10'000'000_nc;
  ostream& debug = cout;
  bool should_prune = true;
};

int main(int argc, char **argv) {
  constexpr int N = 4;
  constexpr int D = 3;
  BoardData<N, D> data;
  State state(data);
  cout << "sizeof(SolutionTree::Node) = " << sizeof(SolutionTree<MiniMax<N, D>::M>::Node) << "\n";
  auto minimax = MiniMax<N, D, PNSearch<N, D, DebugConfig::max_created>, DebugConfig>(state, data);
  auto result = minimax.play(state, Turn::X);
  cout << *result << "\n";
  if (!minimax.get_solution().validate()) {
    cout << "-- VALIDATION FAILED --\n"s;
  }
  if (argc >= 2) {
    auto& solution = minimax.get_solution();
    solution.update_count();
    solution.dump(data, argv[1]);
  }
  return 0;
}
