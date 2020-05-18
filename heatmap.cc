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
  Geometry<5, 3> geom;
  Symmetry sym(geom);
  SymmeTrie trie(sym);
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine generator(seed);
  State state(geom, sym, trie);
  GameEngine b(geom, sym, trie, generator, state);
  b.heat_map();
  return 0;
}

