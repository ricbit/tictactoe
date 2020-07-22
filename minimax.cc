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
  constexpr int D = 2;
  struct DebugConfig {
    int max_nodes = 10000;
    ostream& debug = cout;
  };
  BoardData<N, D> data;
  State state(data);
  auto minimax = MiniMax<N, D, DFS<N, D>, DebugConfig>(state, data);
  auto result = minimax.play(state, Mark::X);
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
