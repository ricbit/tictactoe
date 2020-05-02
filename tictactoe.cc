#include <iostream>
#include <vector>
#include <algorithm>
#include <ctgmath>
#include <random>
#include <chrono>

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
    construct_xor_table();
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
    print(N, [&](int k) {
      return decode(line[k]);
    }, [&](int k) {
      return 'X';
    });
  }

  template<typename X, typename T>
  void print(int limit, X decoder, T func) const {
    static_assert(D == 3);
    vector<vector<vector<char>>> board(N, vector<vector<char>>(
        N, vector<char>(N, '.')));
    for (int k = 0; k < limit; k++) {
      vector<int> decoded = decoder(k);
      board[decoded[0]][decoded[1]][decoded[2]] = func(k);
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
    print(pow(N, D), [&](int k) {
      return decode(k);
    }, [&](int k) {
      int points = accumulation_points[k];
      return encode_points(points);
    });
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

  void construct_xor_table() {
    for (auto& line : winning_lines) {
      int ans = 0;
      for (auto& code : line) {
        ans ^= code;
      }
      xor_table.push_back(ans);
    }
  }

  vector<vector<Direction>> unique_terrains;
  vector<vector<Code>> winning_lines;
  vector<int> accumulation_points;
  vector<vector<int>> winning_positions;
  vector<int> xor_table;
};

enum class Mark {
  X,
  O,
  empty
};

template<int N, int D>
class State {
 public:
  State(const Geometry<N, D>& geom) :
      geom(geom),
      board(pow(N, D), Mark::empty),
      x_lines(geom.winning_lines.size()),
      o_lines(geom.winning_lines.size()),
      open_positions(pow(N, D)),
      xor_table(geom.xor_table) {
  }
  const Geometry<N, D>& geom;
  vector<Mark> board;
  vector<int> x_lines, o_lines;
  int open_positions;
  vector<int> xor_table;

  bool play(Code pos, Mark mark) {
    board[pos] = mark;
    open_positions--;
    for (auto line : geom.winning_positions[pos]) {
      vector<int>& current = get_current(mark);
      current[line]++;
      xor_table[line] ^= pos;
      if (current[line] == N) {
        return true;
      }
    }
    return false;
  }

  vector<Code> get_open_positions() {
    vector<Code> ans;
    for (int i = 0; i < pow(N, D); i++) {
      if (board[i] == Mark::empty) {
        ans.push_back(i);
      }
    }
    return ans;
  }

  vector<int>& get_current(Mark mark) {
    return mark == Mark::X ? x_lines : o_lines;
  }

  vector<int>& get_opponent(Mark mark) {
    return mark == Mark::X ? o_lines : x_lines;
  }

  void print() {
    geom.print(pow(N, D), [&](int k) {
      return geom.decode(k);
    }, [&](int k) {
      return encode_position(board[k]);
    });
  }
  
  char encode_position(Mark pos) {
    return pos == Mark::X ? 'X'
         : pos == Mark::O ? 'O' 
         : '.';
  }

};

template<int N, int D>
class GameEngine {
 public:
  GameEngine(const Geometry<N, D>& geom, 
    default_random_engine& generator,
    vector<int>& search_tree) :
      geom(geom),
      generator(generator),
      state(geom),
      random_position(0, pow(N, D) - 1),
      search_tree(search_tree) {
  }

  bool play(Code pos, Mark mark) {
    search_tree[pow(N, D) - state.open_positions] += state.open_positions;
    return state.play(pos, mark);
  }

  Code random_open_position() {
    vector<Code> pos = state.get_open_positions();
    uniform_int_distribution<int> random_position(0, pos.size() - 1);
    return pos[random_position(generator)];
  }

  optional<Code> find_forcing_move(vector<int>& current, vector<int>& opponent) {
    for (int i = 0; i < static_cast<int>(geom.winning_lines.size()); i++) {
      if (current[i] == N - 1 && opponent[i] == 0) {
        return state.xor_table[i];
      }
    }
    return {};
  }

  Code choose_position(Mark mark) {
    vector<int>& current = state.get_current(mark);
    vector<int>& opponent = state.get_opponent(mark);
    optional<Code> attack = find_forcing_move(current, opponent);
    if (attack.has_value()) {
      return *attack;
    }
    optional<Code> defend = find_forcing_move(opponent, current);
    if (defend.has_value()) {
      return *defend;
    }
    return random_open_position();
  }

  Mark play() {
    Mark current_mark = Mark::X;
    while (state.open_positions) {
      int i = choose_position(current_mark);
      if (play(i, current_mark)) {
        return current_mark;
      }
      current_mark = flip(current_mark);
    }
    return Mark::empty;
  }

  Mark flip(Mark mark) {
    return mark == Mark::X ? Mark::O : Mark::X;
  }

  const Geometry<N, D>& geom;
  default_random_engine& generator;
  State<N, D> state;
  uniform_int_distribution<int> random_position;
  vector<int>& search_tree;
};

int main() {
  Geometry<5, 3> geom;
  vector<int> search_tree(geom.winning_positions.size());
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine generator(seed);
  int max_plays = 10000;
  vector<int> win_counts(3);
  geom.print_points();
  for (int i = 0; i < max_plays; i++) {
    GameEngine b(geom, generator, search_tree);
    win_counts[static_cast<int>(b.play())]++;
    //cout << "\n---\n";
    //b.print();
  }
  double total = 0;
  for (int i = 0; i < static_cast<int>(search_tree.size()); i++) {
    double level = static_cast<double>(search_tree[i]) / max_plays;
    cout << "level " << i << " : " << level << "\n";
    if (level > 0) {
      double log_level = log10(level < 1 ? 1 : level);
      total += log_level;
      cout << log_level << "\n";
    }
  }
  cout << "\ntotal : 10 ^ " << total << "\n";
  cout << "X wins : " << win_counts[static_cast<int>(Mark::X)] << "\n";
  cout << "O wins : " << win_counts[static_cast<int>(Mark::O)] << "\n";
  cout << "draws  : " << win_counts[static_cast<int>(Mark::empty)] << "\n";
  return 0;
}
