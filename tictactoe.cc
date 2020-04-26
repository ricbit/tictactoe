#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

using Code = int;

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
    construct_unique_terrains();
    construct_winning_lines();
  }
  void fill_terrain(vector<Direction>& terrain, int pos) {
    if (pos == D) {
      auto it = find_if(begin(terrain), end(terrain),
          [](auto& elem) { return elem != Direction::equal; });
      if (it != end(terrain) && *it == Direction::up) {
        unique_terrains.push_back(terrain);
      }
      return;
    }
    for (auto dir : all_directions) {
      terrain[pos] = dir;
      fill_terrain(terrain, pos + 1);
    }
  }

  void construct_unique_terrains() {    
    vector<Direction> terrain(D);
    fill_terrain(terrain, 0);
  }

  void generate_lines(const vector<Direction>& terrain, 
      vector<vector<int>> current_line, int pos) {
    if (pos == D) {
      vector<Code> line(N);
      for (int j = 0; j < N; j++) {
        line[j] = encode(current_line[j]);
      }
      winning_lines.push_back(line);
      return;
    }
    switch (terrain[pos]) {
      case Direction::up: 
        for (int i = 0; i < N; i++) {
          current_line[i][pos] = i;
        }
        generate_lines(terrain, current_line, pos + 1);
        break;
      case Direction::down: 
        for (int i = 0; i < N; i++) {
          current_line[i][pos] = N - i - 1;
        }
        generate_lines(terrain, current_line, pos + 1);
        break;
      case Direction::equal:
        for (int j = 0; j < N; j++) {
          for (int i = 0; i < N; i++) {
            current_line[i][pos] = j;
          }
          generate_lines(terrain, current_line, pos + 1);
        }
        break;
    }
  }

  Code encode(vector<int> dim_index) {
    Code ans = 0;
    int factor = 1;
    for (int i = 0; i < D; i++) {
      ans += dim_index[i] * factor;
      factor *= N;
    }
    return ans;
  }

  void construct_winning_lines() {
    vector<vector<int>> current_line(N, vector<int>(D));
    for (auto terrain : unique_terrains) {
      generate_lines(terrain, current_line, 0);
    }
  }

  vector<int> decode(Code code) {
    vector<int> ans;
    for (int i = 0; i < D; i++) {
      ans.push_back(code % N);
      code /= N;
    }
    return ans;
  }

  void print_line(vector<Code>& line) {
    static_assert(D == 3);
    vector<vector<vector<char>>> board(N, vector<vector<char>>(
      N, vector<char>(N, '.')));
    for (int k = 0; k < N; k++) {
      vector<int> decoded = decode(line[k]);
      board[decoded[0]][decoded[1]][decoded[2]] = 'X';
    }
    for (int k = 0; k < N; k++) {
      for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
          cout << board[k][j][i];
        }
        cout << "\n";
      }
      cout << "\n";
    }
  }

  vector<vector<Direction>> unique_terrains;
  vector<vector<Code>> winning_lines;
};

int main() {
  Geometry<5, 3> geom;
  for (auto line : geom.winning_lines) {
    for (auto elem : line) {
      cout << static_cast<int>(elem) << " ";
    }
    cout << "\n";
    geom.print_line(line);
  }
  return 0;
}
