#include <iostream>
#include <vector>
#include <algorithm>
#include <ctgmath>
#include <random>

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
  Geometry() : accumulation_points(pow(N, D)), winning_positions(pow(N, D)) {
    construct_unique_terrains();
    construct_winning_lines();
    construct_accumulation_points();
    construct_winning_positions();
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

  vector<int> decode(Code code) const {
    vector<int> ans;
    for (int i = 0; i < D; i++) {
      ans.push_back(code % N);
      code /= N;
    }
    return ans;
  }

  void construct_accumulation_points() {
    for (const auto& line : winning_lines) {
      for (const auto code : line) {
        accumulation_points[code]++;
      }
    }
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

  void print_points() {
    static_assert(D == 3);
    vector<vector<vector<char>>> board(N, vector<vector<char>>(
        N, vector<char>(N, '.')));
    for (int k = 0; k < pow(N, D); k++) {
      vector<int> decoded = decode(k);
      int points = accumulation_points[k];
      board[decoded[0]][decoded[1]][decoded[2]] = encode_points(points);
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

  char encode_points(int points) {
    return points < 10 ? '0' + points : 
        points < 10 + 26 ? 'A' + points - 10 : '-';
  }

  void construct_winning_positions() {
    int size = static_cast<int>(winning_lines.size());
    for (int i = 0; i < size; i++) {
      for (auto code : winning_lines[i]) {
        winning_positions[code].push_back(i);
      }
    }
  }

  vector<vector<Direction>> unique_terrains;
  vector<vector<Code>> winning_lines;
  vector<int> accumulation_points;
  vector<vector<int>> winning_positions;
};

enum class Mark {
  X,
  O,
  empty
};

template<int N, int D>
class Board {
 public:
  Board(const Geometry<N, D>& geom, vector<int>& search_tree) :
      geom(geom),
      board(pow(N, D), Mark::empty),
      x_lines(geom.winning_lines.size()),
      o_lines(geom.winning_lines.size()),
      generator(1),
      random_position(0, pow(N, D) - 1),
      open_positions(0),
      search_tree(search_tree) {
  }
  bool play(Code pos, Mark mark) {
    board[pos] = mark;
    open_positions++;
    search_tree[pow(N, D) - open_positions] += open_positions;
    for (auto line : geom.winning_positions[pos]) {
      vector<int>& lines = mark == Mark::X ? x_lines : o_lines;
      lines[line]++;
      if (lines[line] == N) {
        return true;
      }
    }
    return false;
  }
  Code random_open_position() {
    while (true) {
      int i = random_position(generator);
      if (board[i] == Mark::empty) {
        return i;
      }
    }
  }
  void play() {
    Mark current_mark = Mark::X;
    while (true) {
      int i = random_open_position();
      if (play(i, current_mark)) {
        cout << "win!\n";
        return;
      }
      current_mark = current_mark == Mark::X ? Mark::O : Mark::X;
    }
  }
  void print() {
    static_assert(D == 3);
    vector<vector<vector<char>>> printing_board(N, vector<vector<char>>(
        N, vector<char>(N, '.')));
    for (int k = 0; k < pow(N, D); k++) {
      vector<int> decoded = geom.decode(static_cast<Code>(k));
      printing_board[decoded[0]][decoded[1]][decoded[2]] =
          encode_position(board[k]);
    }
    for (int k = 0; k < N; k++) {
      for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
          cout << printing_board[k][j][i];
        }
        cout << "\n";
      }
      cout << "\n";
    }
  }
  char encode_position(Mark pos) {
    return pos == Mark::X ? 'X'
        : pos == Mark::O ? 'O' 
        : '.';
  }
  const Geometry<N, D>& geom;
  vector<Mark> board;
  vector<int> x_lines, o_lines;
  default_random_engine generator;
  uniform_int_distribution<int> random_position;
  int open_positions;
  vector<int>& search_tree;
};

int main() {
  Geometry<3, 3> geom;
  for (auto& line : geom.winning_lines) {
    for (auto elem : line) {
      cout << static_cast<int>(elem) << " ";
    }
    cout << "\n";
    geom.print_line(line);
  }
  geom.print_points();
  for (auto& lines : geom.winning_positions) {
    for (auto line : lines) {
      cout << line << " ";
    }
    cout << "\n";
  }
  vector<int> search_tree(pow(N, D));
  Board b(geom, search_tree);
  b.play();
  b.print();
  return 0;
}
