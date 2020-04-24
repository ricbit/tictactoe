#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

using code = int;

enum class Direction {
  equal,
  up,
  down
};

constexpr initializer_list<Direction> all_directions {
  Direction::equal, 
  Direction::up, 
  Direction::down
};

template<int N, int D>
class Geometry {
 public:
  Geometry() {
    construct_winning_lines();
  }
  void fill_terrain(vector<Direction>& terrain, int pos) {
    if (pos == D) {
      auto it = find_if(begin(terrain), end(terrain),
          [](auto& elem) { return elem != Direction::equal; });
      if (it != end(terrain) && *it == Direction::up) {
        winning_lines.push_back(terrain);
      }
      return;
    }
    for (auto dir : all_directions) {
      terrain[pos] = dir;
      fill_terrain(terrain, pos + 1);
    }
  }
  void construct_winning_lines() {    
    vector<Direction> terrain(D);
    fill_terrain(terrain, 0);
  }
  vector<vector<Direction>> winning_lines;
};

int main() {
  Geometry<5, 3> geom;
  for (auto line : geom.winning_lines) {
    for (auto elem : line) {
      cout << static_cast<int>(elem) << " ";
    }
    cout << "\n";
  }
  return 0;
}
