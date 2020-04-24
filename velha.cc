#include <iostream>
#include <vector>
#include <cstdlib>

using namespace std;

using code = int;

enum Direction {
  equal = 0,
  up = 1,
  down = 2
};

vector<Direction> all_directions{
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
  void fill_terrain(vector<int>& terrain, int pos) {
    if (pos == D) {
      winning_lines.push_back(terrain);
    }
    for (auto dir : all_directions) {
      terrain[pos] = dir;
      fill_terrain(terrain, pos + 1);
    }
  }
  void construct_winning_lines() {    
    for (int d = 0; d < D; d++) {
      vector<int> terrain(D);
      fill_terrain(terrain, 0);
    }
  }
  vector<vector<code>> winning_lines;
};

int main() {
  Geometry<3, 2> geom;
  for (auto line : geom.winning_lines) {
    for (auto elem : line) {
      cout << elem << " ";
    }
    cout << "\n";
  }
  return 0;
}
