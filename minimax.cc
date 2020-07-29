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
  constexpr int N = 5;
  constexpr int D = 3;
  struct DebugConfig {
    NodeCount max_visited = 10'000'000_nc;
    NodeCount max_created = 1'000'000_nc;
    ostream& debug = cout;
    bool should_prune = false;
  };
  BoardData<N, D> data;
  State state(data);
  cout << "sizeof(SolutionTree::Node) = " << sizeof(SolutionTree::Node) << "\n";
  auto minimax = MiniMax<N, D, PNSearch<N, D>, DebugConfig>(state, data);
  auto result = minimax.play(state, Turn::X);
  cout << *result << "\n";
  if (!minimax.get_solution().validate()) {
    cout << "-- VALIDATION FAILED --\n"s;
  }
  if (argc >= 2) {
    SolutionTree& solution = minimax.get_solution();
    solution.update_count();
    solution.dump(data, argv[1]);
  }
  return 0;
}
