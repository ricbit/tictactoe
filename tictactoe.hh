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
#include "semantic.hh"

using namespace std;

SEMANTIC_INDEX(Position, pos)
SEMANTIC_INDEX(Side, side)
SEMANTIC_INDEX(Line, line)
SEMANTIC_INDEX(Dim, dim)
SEMANTIC_INDEX(SymLine, sym)
SEMANTIC_INDEX(NodeLine, node)
SEMANTIC_INDEX(LineCount, lcount)
SEMANTIC_INDEX(MarkCount, mcount)

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

template<typename T>
constexpr T pow(T a, T b) {
  int ans = 1;
  for (int i = 0; i < b; i++) {
    ans *= a;
  }
  return ans;
}

template<int N, int D>
class Geometry {
 public:
  Geometry()
      : _accumulation_points(board_size), _lines_through_position(board_size) {
    construct_unique_terrains();
    construct_winning_lines();
    construct_accumulation_points();
    construct_lines_through_position();
    construct_xor_table();
  }

  constexpr static Position board_size =
      static_cast<Position>(pow(N, D));

  constexpr static Line line_size =
      static_cast<Line>((pow(N + 2, D) - pow(N, D)) / 2);

  const vector<vector<Line>>& lines_through_position() const {
    return _lines_through_position;
  }

  const vector<vector<Position>>& winning_lines() const {
    return _winning_lines;
  }

  const vector<LineCount>& accumulation_points() const {
    return _accumulation_points;
  }

  const svector<Line, Position>& xor_table() const {
    return _xor_table;
  }

  sarray<Dim, Side, D> decode(Position pos) const {
    sarray<Dim, Side, D> ans;
    for (Dim i = 0_dim; i < D; ++i) {
      ans[i] = Side{pos % N};
      pos /= N;
    }
    return ans;
  }

  void apply_permutation(const vector<Position>& source,
      vector<Position>& dest, const vector<Side>& permutation) const {
    transform(begin(source), end(source), begin(dest), [&](Position pos) {
      return apply_permutation(permutation, pos);
    });
  }

  void print_line(const vector<Position>& line) {
    print(N, [&](Line k) {
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
    for (Position k = 0_pos; k < limit; ++k) {
      auto decoded = decoder(k);
      board[decoded[0_dim]][decoded[1_dim]][decoded[2_dim]] = func(k);
    }
    for (Side k = 0_side; k < N; ++k) {
      for (Side j = 0_side; j < N; ++j) {
        for (Side i = 0_side; i < N; ++i) {
          cout << board[k][j][i];
        }
        cout << " ";
      }
      cout << "\n";
    }
  }

  void print_points() {
    print(board_size, [&](Position k) {
      return decode(k);
    }, [&](Position k) {
      int points = _accumulation_points[k];
      return encode_points(points);
    });
  }

 private:
  Position apply_permutation(const vector<Side>& permutation, Position pos) const {
    auto decoded = decode(pos);
    transform(begin(decoded), end(decoded), begin(decoded), [&](Side x) {
      return permutation[x];
    });
    return encode(decoded);
  }

  void fill_terrain(vector<Direction>& terrain, Dim dim) {
    if (dim == D) {
      auto it = find_if(begin(terrain), end(terrain),
          [](const auto& elem) { return elem != Direction::equal; });
      if (it != end(terrain) && *it == Direction::up) {
        unique_terrains.push_back(terrain);
      }
      return;
    }
    for (auto dir : all_directions) {
      terrain[dim] = dir;
      fill_terrain(terrain, Dim{dim + 1});
    }
  }

  void construct_unique_terrains() {
    vector<Direction> terrain(D);
    fill_terrain(terrain, 0_dim);
  }

  void generate_lines(const vector<Direction>& terrain,
      vector<sarray<Dim, Side, D>> current_line, Dim dim) {
    if (dim == D) {
      vector<Position> line(N);
      transform(begin(current_line), end(current_line), begin(line),
          [&](const auto& line) {
        return encode(line);
      });
      sort(begin(line), end(line));
      _winning_lines.push_back(line);
      return;
    }
    switch (terrain[dim]) {
      case Direction::up:
        for (Side i = 0_side; i < N; ++i) {
          current_line[i][dim] = Side{i};
        }
        generate_lines(terrain, current_line, Dim{dim + 1});
        break;
      case Direction::down:
        for (Side i = 0_side; i < N; ++i) {
          current_line[i][dim] = Side{N - i - 1};
        }
        generate_lines(terrain, current_line, Dim{dim + 1});
        break;
      case Direction::equal:
        for (Side j = 0_side; j < N; ++j) {
          for (Side i = 0_side; i < N; ++i) {
            current_line[i][dim] = Side{j};
          }
          generate_lines(terrain, current_line, Dim{dim + 1});
        }
        break;
    }
  }

  Position encode(const sarray<Dim, Side, D>& dim_index) const {
    Position ans = 0_pos;
    int factor = 1;
    for (auto index : dim_index) {
      ans += index * factor;
      factor *= N;
    }
    return ans;
  }

  void construct_winning_lines() {
    vector<sarray<Dim, Side, D>> current_line(N, sarray<Dim, Side, D>(0_side));
    for (const auto& terrain : unique_terrains) {
      generate_lines(terrain, current_line, 0_dim);
    }
    sort(begin(_winning_lines), end(_winning_lines));
    assert(static_cast<int>(_winning_lines.size()) == line_size);
  }

  void construct_accumulation_points() {
    for (const auto& line : _winning_lines) {
      for (const auto pos : line) {
        _accumulation_points[pos]++;
      }
    }
  }

  char encode_points(int points) const {
    return points < 10 ? '0' + points :
        points < 10 + 26 ? 'A' + points - 10 : '-';
  }

  void construct_lines_through_position() {
    for (Line i = 0_line; i < line_size; ++i) {
      for (auto pos : _winning_lines[i]) {
        _lines_through_position[pos].push_back(i);
      }
    }
  }

  void construct_xor_table() {
    for (const auto& line : _winning_lines) {
      Position ans = accumulate(begin(line), end(line), 0_pos,
          [](auto x, auto y) {
        return Position{x ^ y};
      });
      _xor_table.push_back(ans);
    }
  }

  vector<vector<Direction>> unique_terrains;
  vector<vector<Position>> _winning_lines;
  vector<LineCount> _accumulation_points;
  vector<vector<Line>> _lines_through_position;
  svector<Line, Position> _xor_table;
};

enum class Mark {
  X,
  O,
  empty
};

template<int N, int D>
class Symmetry {
 public:
  explicit Symmetry(const Geometry<N, D>& geom)
      : geom(geom) {
    generate_all_rotations();
    generate_all_eviscerations();
    multiply_groups();
  }

  constexpr static Position board_size = Geometry<N, D>::board_size;

  const vector<vector<Position>>& symmetries() const {
    return _symmetries;
  }

 private:
  void multiply_groups() {
    set<vector<Position>> unique;
    for (const auto& rotation : rotations) {
      for (const auto& evisceration : eviscerations) {
        vector<Position> symmetry(geom.board_size);
        for (Position i = 0_pos; i < geom.board_size; ++i) {
          symmetry[rotation[evisceration[i]]] = i;
        }
        unique.insert(symmetry);
      }
    }
    copy(begin(unique), end(unique), back_inserter(_symmetries));
    sort(begin(_symmetries), end(_symmetries));
  }

  void generate_all_eviscerations() {
    vector<Side> index(N);
    iota(begin(index), end(index), 0_side);
    do {
      if (validate_evisceration(index)) {
        generate_evisceration(index);
      }
    } while (next_permutation(begin(index), end(index)));
  }

  void generate_evisceration(const vector<Side>& index) {
    vector<Position> symmetry(geom.board_size);
    iota(begin(symmetry), end(symmetry), 0_pos);
    geom.apply_permutation(symmetry, symmetry, index);
    eviscerations.push_back(symmetry);
  }

  bool validate_evisceration(const vector<Side>& index) {
    for (const auto& line : geom.winning_lines()) {
      vector<Position> transformed(N);
      geom.apply_permutation(line, transformed, index);
      sort(begin(transformed), end(transformed));
      if (!search_line(transformed)) {
        return false;
      }
    }
    return true;
  }

  bool search_line(const vector<Position>& transformed) {
    return binary_search(
        begin(geom.winning_lines()), end(geom.winning_lines()), transformed);
  }

  void generate_all_rotations() {
    vector<Dim> index(D);
    iota(begin(index), end(index), 0_dim);
    do {
      for (int i = 0; i < (1 << D); ++i) {
        rotations.push_back(generate_rotation(index, i));
      }
    } while (next_permutation(begin(index), end(index)));
  }

  vector<Position> generate_rotation(const vector<Dim>& index, int bits) {
    vector<Position> symmetry;
    for (Position i = 0_pos; i < geom.board_size; ++i) {
      auto decoded = geom.decode(i);
      Position ans = 0_pos;
      int current_bits = bits;
      for (auto side : index) {
        auto column = decoded[side];
        ans = Position{ans * N +
            ((current_bits & 1) == 0 ? column : N - column - 1)};
        current_bits >>= 1;
      }
      symmetry.push_back(ans);
    }
    return symmetry;
  }

  void print_symmetry(const vector<Position>& symmetry) {
    geom.print(geom.board_size, [&](Position k) {
      return geom.decode(k);
    }, [&](SymLine k) {
      return geom.encode_points(symmetry[k]);
    });
  }

  void print() {
    for (const auto& symmetry : _symmetries) {
      print_symmetry(symmetry);
      cout << "\n---\n\n";
    }
  }

  const Geometry<N, D>& geom;
  vector<vector<Position>> _symmetries;
  vector<vector<Position>> rotations;
  vector<vector<Position>> eviscerations;
};

template<int N, int D>
class SymmeTrie {
 public:
  explicit SymmeTrie(const Symmetry<N, D>& sym) : sym(sym) {
    construct_trie();
    construct_mask();
  }

  constexpr static int board_size = Symmetry<N, D>::board_size;
  using Bitfield = bitset<board_size>;

  const vector<SymLine>& similar(NodeLine line) const {
    return nodes[line].similar;
  }

  NodeLine next(NodeLine line, Position pos) const {
    return nodes[line].next[pos];
  }

  const Bitfield& mask(NodeLine line, Position pos) const {
    return nodes[line].mask[pos];
  }

  void print() {
    for (const auto& node : nodes) {
      cout << " --- \n";
      print_node(node);
      for (Position j = 0_pos; j < board_size; ++j) {
        cout << j << " -> ";
        print_node(nodes[node.next[j]]);
      }
    }
  }

 private:
  struct Node {
    explicit Node(const vector<SymLine>& similar)
        : similar(similar), next(board_size), mask(board_size) {
    }
    vector<SymLine> similar;
    vector<NodeLine> next;
    vector<Bitfield> mask;
  };

  const Symmetry<N, D>& sym;
  vector<Node> nodes;

  void construct_mask() {
    for (auto& node : nodes) {
      for (Position pos = 0_pos; pos < board_size; ++pos) {
        node.mask[pos].reset();
        for (SymLine line : node.similar) {
          node.mask[pos].set(sym.symmetries()[line][pos]);
        }
      }
    }
  }

  void print_node(const Node& node) {
    for (auto index : node.similar) {
      cout << index << " ";
    }
    cout << "\n";
  }

  void construct_trie() {
    vector<SymLine> root(sym.symmetries().size());
    iota(begin(root), end(root), 0_sym);
    queue<NodeLine> pool;
    nodes.push_back(Node(root));
    pool.push(0_node);
    while (!pool.empty()) {
      auto current_node = pool.front();
      vector<SymLine> current = nodes[current_node].similar;
      const SymLine current_size = static_cast<SymLine>(current.size());
      pool.pop();
      for (Position i = 0_pos; i < board_size; ++i) {
        vector<SymLine> next_similar;
        for (SymLine j = 0_sym; j < current_size; ++j) {
          if (i == sym.symmetries()[current[j]][i]) {
            next_similar.push_back(j);
          }
        }
        auto it = find_if(begin(nodes), end(nodes), [&](const auto& x) {
          return x.similar == next_similar;
        });
        if (it == end(nodes)) {
          nodes.push_back(Node(next_similar));
          NodeLine last_node = static_cast<NodeLine>(nodes.size() - 1);
          pool.push(last_node);
          nodes[current_node].next[i] = last_node;
        } else {
          nodes[current_node].next[i] =
              static_cast<NodeLine>(distance(begin(nodes), it));
        }
      }
    }
  }
};

template<int N, int D>
class State {
 public:
  State(const Geometry<N, D>& geom, const Symmetry<N, D>& sym,
    const SymmeTrie<N, D>& trie) :
      geom(geom),
      sym(sym),
      trie(trie),
      board(Mark::empty),
      x_marks_on_line(geom.line_size),
      o_marks_on_line(geom.line_size),
      xor_table(geom.xor_table()),
      active_line(geom.line_size, true),
      current_accumulation(geom.accumulation_points()),
      trie_node(0_node),
      empty_cells(board_size) {
    iota(begin(empty_cells), end(empty_cells), 0_pos);
    for (auto it = begin(empty_cells); it != end(empty_cells); ++it) {
      empty_it.push_back(it);
    }
  }

  using Bitfield = typename SymmeTrie<N, D>::Bitfield;

  const Bitfield& get_open_positions(Mark mark) {
    open_positions.reset();
    checked.reset();
    for (Position i : empty_cells) {
      if (!checked[i]) {
        open_positions[i] = true;
        checked |= trie.mask(trie_node, i);
      }
    }
    return open_positions;
  }

  auto& get_current(Mark mark) {
    return mark == Mark::X ? x_marks_on_line : o_marks_on_line;
  }

  auto& get_opponent(Mark mark) {
    return mark == Mark::X ? o_marks_on_line : x_marks_on_line;
  }

  void print() {
    geom.print(geom.board_size, [&](Position k) {
      return geom.decode(k);
    }, [&](Position k) {
      return encode_position(board[k]);
    });
  }

  bool play(Position pos, Mark mark) {
    board[pos] = mark;
    empty_cells.erase(empty_it[pos]);
    empty_it[pos] = end(empty_cells);
    trie_node = trie.next(trie_node, pos);
    for (Line line : geom.lines_through_position()[pos]) {
      auto& current = get_current(mark);
      current[line]++;
      xor_table[line] ^= pos;
      if (current[line] == N) {
        return true;
      }
      if (x_marks_on_line[line] > 0 &&
          o_marks_on_line[line] > 0 &&
          active_line[line]) {
        active_line[line] = false;
        for (Side j = 0_side; j < N; ++j) {
          Position neigh = geom.winning_lines()[line][j];
          current_accumulation[neigh]--;
          if (current_accumulation[neigh] == 0 &&
              empty_it[neigh] != end(empty_cells)) {
            empty_cells.erase(empty_it[neigh]);
            empty_it[neigh] = end(empty_cells);
          }
        }
      }
    }
    return false;
  }

  Position get_xor_table(Line line) {
    return xor_table[line];
  }

  LineCount get_current_accumulation(Position pos) {
    return current_accumulation[pos];
  };

 private:
  constexpr static int board_size = Geometry<N, D>::board_size;
  const Geometry<N, D>& geom;
  const Symmetry<N, D>& sym;
  const SymmeTrie<N, D>& trie;
  sarray<Position, Mark, board_size> board;
  svector<Line, MarkCount> x_marks_on_line, o_marks_on_line;
  svector<Line, Position> xor_table;
  vector<bool> active_line;
  vector<LineCount> current_accumulation;
  Bitfield open_positions;
  NodeLine trie_node;
  Bitfield checked;
  using EmptyList = list<Position>;
  EmptyList empty_cells;
  vector<EmptyList::iterator> empty_it;

  void print_accumulation() {
    geom.print(geom.board_size, [&](Position k) {
      return geom.decode(k);
    }, [&](Position k) {
      return geom.encode_points(current_accumulation[k]);
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
    const Symmetry<N, D>& sym,
    const SymmeTrie<N, D>& trie,
    default_random_engine& generator) :
      geom(geom),
      sym(sym),
      trie(trie),
      generator(generator),
      state(geom, sym, trie) {
  }

  using Bitfield = typename State<N, D>::Bitfield;
  constexpr static Position board_size = Geometry<N, D>::board_size;

  bool play(Position pos, Mark mark) {
    return state.play(pos, mark);
  }

  Position random_open_position(const Bitfield& open_positions) {
    int total = 0;
    for (Position pos = 0_pos; pos < board_size; ++pos) {
      if (open_positions[pos]) {
        total += state.get_current_accumulation(pos);
      }
    }
    uniform_int_distribution<int> random_position(0, total - 1);
    int chosen = random_position(generator);
    int previous = 0;
    for (Position pos = 0_pos; pos < board_size; ++pos) {
      if (open_positions[pos]) {
        int current = previous + state.get_current_accumulation(pos);
        if (chosen < current) {
          return pos;
        }
        previous = current;
      }
    }
    assert(false);
  }

  optional<Position> find_forcing_move(
      const svector<Line, MarkCount>& current,
      const svector<Line, MarkCount>& opponent,
      const Bitfield& open_positions) {
    for (Line i = 0_line; i < geom.line_size; ++i) {
      if (current[i] == N - 1 && opponent[i] == 0) {
        Position trial = state.get_xor_table(i);
        if (open_positions[trial]) {
          return trial;
        }
      }
    }
    return {};
  }

  Position choose_position(Mark mark, const Bitfield& open_positions) {
    const auto& current = state.get_current(mark);
    const auto& opponent = state.get_opponent(mark);
    optional<Position> attack =
        find_forcing_move(current, opponent, open_positions);
    if (attack.has_value()) {
      return *attack;
    }
    optional<Position> defend =
        find_forcing_move(opponent, current, open_positions);
    if (defend.has_value()) {
      return *defend;
    }
    return random_open_position(open_positions);
  }

  template<typename T>
  Mark play(T observer) {
    Mark current_mark = Mark::X;
    while (true) {
      const auto& open_positions = state.get_open_positions(current_mark);
      if (open_positions.none()) {
        return Mark::empty;
      }
      observer(open_positions);
      Position pos = choose_position(current_mark, open_positions);
      auto result = play(pos, current_mark);
      /*state.print();
      state.print_accumulation();
      cout << "\n------\n\n";*/
      if (result) {
        return current_mark;
      }
      current_mark = flip(current_mark);
    }
  }

  Mark flip(Mark mark) {
    return mark == Mark::X ? Mark::O : Mark::X;
  }

  void heat_map() {
    Bitfield open = state.get_open_positions(Mark::X);
    const int trials = 100;
    //for (Pos
    state.print();
  }

  const Geometry<N, D>& geom;
  const Symmetry<N, D>& sym;
  const SymmeTrie<N, D>& trie;
  default_random_engine& generator;
  State<N, D> state;
};

int new_main() {
  Geometry<5, 3> geom;
  Symmetry sym(geom);
  SymmeTrie trie(sym);
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine generator(seed);
  GameEngine b(geom, sym, trie, generator);
  b.heat_map();
  return 0;
}

