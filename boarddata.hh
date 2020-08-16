#ifndef BOARDDATA_HH
#define BOARDDATA_HH

#include <iostream>
#include <iomanip>
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
#include <limits>
#include <ranges>
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
SEMANTIC_INDEX(Crossing, cross)
SEMANTIC_INDEX(ProofNumber, pn)
SEMANTIC_INDEX(NodeCount, nc)

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
  T ans = T{1};
  for (int i = 0; i < b; i++) {
    ans *= a;
  }
  return ans;
}

template<typename T>
constexpr T factorial(T a) {
  T ans = T{1};
  for (int i = 1; i <= a; i++) {
    ans *= i;
  }
  return ans;
}

enum class Mark {
  empty = 0,
  X = 1,
  O = 2,
  both = 3
};

ostream& operator<<(ostream& oss, const Mark& value) {
  switch (value) {
    case Mark::empty:
      oss << "empty"s;
      break;
    case Mark::X:
      oss << "X"s;
      break;
    case Mark::O:
      oss << "O"s;
      break;
    case Mark::both:
      oss << "both"s;
      break;
  }
  return oss;
}

enum class Turn {
  X = 0,
  O = 1
};

ostream& operator<<(ostream& oss, const Turn& value) {
  switch (value) {
    case Turn::X:
      oss << "X"s;
      break;
    case Turn::O:
      oss << "O"s;
      break;
  }
  return oss;
}

Mark to_mark(Turn turn) {
  return static_cast<Mark>(static_cast<int>(turn) + 1);
}

Turn to_turn(Mark mark) {
  return static_cast<Turn>(static_cast<int>(mark) - 1);
}

enum class BoardValue {
  #include "boardvalue.hh"
};

ostream& operator<<(ostream& oss, const BoardValue& value) {
  switch (value) {
    case BoardValue::X_WIN:
      oss << "X wins"s;
      break;
    case BoardValue::O_WIN:
      oss << "O wins"s;
      break;
    case BoardValue::DRAW:
      oss << "Draw"s;
      break;
    case BoardValue::UNKNOWN:
      oss << "Unknown"s;
      break;
  }
  return oss;
}

template<typename T>
ostream& operator<<(ostream& oss, const optional<T>& value) {
  if (value.has_value()) {
    oss << *value;
  } else {
    oss << "{}"s;
  }
  return oss;
}

Mark flip(Mark mark) {
  return static_cast<Mark>(static_cast<int>(mark) ^ 3);
}

Turn flip(Turn turn) {
  return static_cast<Turn>(static_cast<int>(turn) ^ 1);
}

using Zobrist = uint64_t;

template<int N, int D>
class Geometry {
 public:
  constexpr static int zobrist_seed = 1;
  Geometry()
      : _accumulation_points(0_lcount),
        _lines_through_position(board_size),
        current_winning(0_line),
        zobrist_generator(zobrist_seed) {
    construct_unique_terrains();
    construct_winning_lines();
    construct_accumulation_points();
    construct_lines_through_position();
    construct_xor_table();
    construct_crossings();
    construct_zobrist();
  }

  constexpr static Position board_size =
      static_cast<Position>(pow(N, D));

  constexpr static Line line_size =
      static_cast<Line>((pow(N + 2, D) - pow(N, D)) / 2);

  const vector<vector<Line>>& lines_through_position() const {
    return _lines_through_position;
  }

  using CrossingArray = sarray<Position, vector<pair<Line, Line>>, board_size>;
  using WinningArray = sarray<Line, sarray<Side, Position, N>, line_size>;
  using SideArray = sarray<Dim, Side, D>;

  const WinningArray& winning_lines() const {
    return _winning_lines;
  }

  const sarray<Position, LineCount, board_size>& accumulation_points() const {
    return _accumulation_points;
  }

  const sarray<Line, Position, line_size>& xor_table() const {
    return _xor_table;
  }

  const CrossingArray& crossings() const {
    return _crossings;
  }

  const auto& zobrist_x() const {
    return _zobrist_x;
  }

  const auto& zobrist_o() const {
    return _zobrist_o;
  }

  SideArray decode(Position pos) const {
    SideArray ans;
    for (Dim i = 0_dim; i < D; ++i) {
      ans[i] = Side{pos % N};
      pos /= N;
    }
    return ans;
  }

  template<typename T>
  void apply_permutation(const T& source, T& dest,
      const vector<Side>& permutation) const {
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
    if constexpr (D == 3) {
      print3(limit, decoder, func);
    } else if constexpr (D == 2) {
      print2(limit, decoder, func);
    } else {
      static_assert(D == 3 || D == 2);
    }
  }

  template<typename X, typename T>
  void print3(int limit, X decoder, T func) const {
    static_assert(D == 3);
    vector<vector<vector<string>>> board(N, vector<vector<string>>(
        N, vector<string>(N, "."s)));
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

  template<typename X, typename T>
  void print2(int limit, X decoder, T func) const {
    static_assert(D == 2);
    vector<vector<string>> board(N, vector<string>(N, "."s));
    for (Position k = 0_pos; k < limit; ++k) {
      auto decoded = decoder(k);
      board[decoded[0_dim]][decoded[1_dim]] = func(k);
    }
    for (Side j = 0_side; j < N; ++j) {
      for (Side i = 0_side; i < N; ++i) {
        cout << board[j][i];
      }
      cout << "\n";
    }
    cout << "\n";
  }

  void print_points() {
    print(board_size, [&](Position k) {
      return decode(k);
    }, [&](Position k) {
      int points = _accumulation_points[k];
      return encode_points(points);
    });
  }

  Position encode(const SideArray& dim_index) const {
    Position ans = 0_pos;
    int factor = 1;
    for (auto index : dim_index) {
      ans += index * factor;
      factor *= N;
    }
    return ans;
  }

  char encode_points(int points) const {
    return points < 10 ? '0' + points :
        points < 10 + 26 ? 'A' + points - 10 : '-';
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
      sarray<Side, Position, N> line;
      transform(begin(current_line), end(current_line), begin(line),
          [&](const auto& line) {
        return encode(line);
      });
      sort(begin(line), end(line));
      _winning_lines[current_winning] = line;
      ++current_winning;
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

  void construct_lines_through_position() {
    for (Line i = 0_line; i < line_size; ++i) {
      for (auto pos : _winning_lines[i]) {
        _lines_through_position[pos].push_back(i);
      }
    }
  }

  void construct_xor_table() {
    auto xor_func = [](const auto& line) {
      return accumulate(begin(line), end(line), 0_pos, [](auto x, auto y) {
        return Position{x ^ y};
      });
    };
    transform(
      begin(_winning_lines), end(_winning_lines),
      begin(_xor_table), xor_func);
  }

  void construct_crossings() {
    for (Position pos = 0_pos; pos < board_size; ++pos) {
      for (Line a : _lines_through_position[pos]) {
        for (Line b : _lines_through_position[pos]) {
          if (a >= b) {
            continue;
          }
          _crossings[pos].push_back(make_pair(a, b));
        }
      }
    }
  }

  void construct_zobrist() {
    uniform_int_distribution<Zobrist> dist(
        numeric_limits<Zobrist>::min(),
        numeric_limits<Zobrist>::max());
    assert(numeric_limits<Zobrist>::min() >= dist.min());
    assert(numeric_limits<Zobrist>::max() <= dist.max());
    construct_zobrist_array(dist, _zobrist_x);
    construct_zobrist_array(dist, _zobrist_o);
  }

  template<typename Array, typename Dist>
  void construct_zobrist_array(Dist& dist, Array& zobrist) {
    generate(begin(zobrist), end(zobrist), [&]() {
      return dist(zobrist_generator);
    });
  }

  vector<vector<Direction>> unique_terrains;
  WinningArray _winning_lines;
  sarray<Position, LineCount, board_size> _accumulation_points;
  vector<vector<Line>> _lines_through_position;
  sarray<Line, Position, line_size> _xor_table;
  sarray<Position, vector<pair<Line, Line>>, board_size> _crossings;
  sarray<Position, Zobrist, board_size> _zobrist_x, _zobrist_o;
  Line current_winning;
  default_random_engine zobrist_generator;
};

template<int N, int D>
class Bitfield {
 public:
  bool operator[](Position pos) {
    return bitfield[pos];
  }
  bool operator[](Position pos) const {
    return bitfield[pos];
  }
  auto count() const {
    return bitfield.count();
  }
  auto& operator|=(const Bitfield& that) {
    bitfield |= that.bitfield;
    return *this;
  }
  void set(Position pos) {
    bitfield.set(pos);
  }
  void reset() {
    bitfield.reset();
  }
  bool none() const {
    return bitfield.none();
  }
  class Iterator {
    const Bitfield<N, D>& instance;
    Position pos;
   public:
    Iterator(const Bitfield<N, D>& instance, Position pos)
        : instance(instance), pos(pos) {
    }
    bool operator!=(const Iterator& that) const {
      return pos != that.pos;
    }
    Position operator*() const {
      return pos;
    }
    Iterator& operator++() {
      pos = instance.get_next_bit(Position{pos + 1});
      return *this;
    }
  };
  Iterator begin() const {
    return Iterator{*this, get_next_bit(0_pos)};
  }
  Iterator end() const {
    return Iterator{*this, board_size};
  }
  auto all() const {
    return *this;
    /*return
        views::iota(0, static_cast<int>(board_size)) |
        views::filter([&](auto pos) { return bitfield[pos]; }) |
        views::transform([](auto pos) { return Position{pos}; });*/
  }
  vector<Position> get_vector() const {
    vector<Position> ans;
    ans.reserve(board_size);
    for (Position pos : *this) {
      ans.push_back(pos);
    }
    return ans;
  }
  bool operator==(const Bitfield<N, D>& that) const {
    return bitfield == that.bitfield;
  }
  Bitfield<N, D> operator&(const Bitfield<N, D>& that) const {
    return Bitfield<N, D>{bitfield & that.bitfield};
  }
  Bitfield<N, D>() {
  }

 private:
  Position get_next_bit(Position pos) const {
    for (; pos < board_size; ++pos) {
      if (bitfield[pos]) {
        return pos;
      }
    }
    return pos;
  }
  constexpr static Position board_size = Geometry<N, D>::board_size;
  Bitfield<N, D>(const bitset<board_size>& bits) : bitfield(bits) {
  }
  bitset<board_size> bitfield;
};

template<int N, int D>
class Symmetry {
 public:
  explicit Symmetry(const Geometry<N, D>& geom)
      : geom(geom) {
    generate_all_rotations();
    generate_all_eviscerations();
    multiply_groups();
    assert(_symmetries.size() == symmetries_size);
  }

  constexpr static Position board_size = Geometry<N, D>::board_size;
  constexpr static SymLine symmetries_size =
      SymLine{pow(2, D - 1 + (N >> 1)) * factorial(D) * factorial(N >> 1)};

  const vector<vector<Position>>& symmetries() const {
    return _symmetries;
  }

  void dump_symmetries() const {
    int line = 0;
    for (const auto& board : _symmetries) {
      cout << setw(2) << line++ << " : ";
      for (Position pos : board) {
        cout << setw(2) << static_cast<int>(pos) << " ";
      }
      cout << "\n";
    }
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
      sarray<Side, Position, N> transformed;
      geom.apply_permutation(line, transformed, index);
      sort(begin(transformed), end(transformed));
      if (!search_line(transformed)) {
        return false;
      }
    }
    return true;
  }

  bool search_line(const sarray<Side, Position, N>& transformed) {
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

  constexpr static Position board_size = Symmetry<N, D>::board_size;

  const vector<SymLine>& similar(NodeLine line) const {
    return nodes[line].similar;
  }

  void dump_similar(NodeLine line) const {
    for (auto symline : similar(line)) {
      cout << symline << " ";
    }
    cout << "\n";
  }

  NodeLine next(NodeLine line, Position pos) const {
    return nodes[line].next[pos];
  }

  const Bitfield<N, D>& mask(NodeLine line, Position pos) const {
    return nodes[line].mask[pos];
  }

  bool is_identity(const NodeLine line) const {
    return nodes[line].similar.size() == 1;
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
  SymLine size() const {
    return static_cast<SymLine>(nodes.size());
  }

 private:
  struct Node {
    explicit Node(const vector<SymLine>& similar)
        : similar(similar), next(board_size), mask(board_size) {
    }
    vector<SymLine> similar;
    vector<NodeLine> next;
    vector<Bitfield<N, D>> mask;
  };

  const Symmetry<N, D>& sym;
  vector<Node> nodes;

  void construct_mask() {
    transform(begin(nodes), end(nodes), begin(nodes), [&](Node& node) {
      for (Position pos = 0_pos; pos < board_size; ++pos) {
        node.mask[pos].reset();
        for (SymLine line : node.similar) {
          node.mask[pos].set(sym.symmetries()[line][pos]);
        }
      }
      return node;
    });
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
      pool.pop();
      for (Position i = 0_pos; i < board_size; ++i) {
        bag<SymLine> next_similar;
        for (SymLine line : current) {
          if (i == sym.symmetries()[line][i]) {
            next_similar.push_back(line);
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
class BoardData {
 public:
  BoardData() : sym(geom), trie(sym) {
  }

  constexpr static Position board_size = Geometry<N, D>::board_size;
  constexpr static Line line_size = Geometry<N, D>::line_size;

  using CrossingArray = typename Geometry<N, D>::CrossingArray;
  using WinningArray = typename Geometry<N, D>::WinningArray;

  template<typename X, typename T>
  void print(int limit, X decoder, T func) const {
    geom.print(limit, decoder, func);
  }

  const vector<SymLine>& similar(NodeLine line) const {
    return trie.similar(line);
  }

  NodeLine next(NodeLine line, Position pos) const {
    return trie.next(line, pos);
  }

  const Bitfield<N, D>& mask(NodeLine line, Position pos) const {
    return trie.mask(line, pos);
  }

  const sarray<Position, LineCount, board_size>& accumulation_points() const {
    return geom.accumulation_points();
  }

  const sarray<Line, Position, line_size>& xor_table() const {
    return geom.xor_table();
  }

  const vector<vector<Line>>& lines_through_position() const {
    return geom.lines_through_position();
  }

  const WinningArray& winning_lines() const {
    return geom.winning_lines();
  }

  const int symmetries_size() const {
    return sym.symmetries().size();
  }

  const CrossingArray& crossings() const {
    return geom.crossings();
  }

  const sarray<Dim, Side, D> decode(Position pos) const {
    return geom.decode(pos);
  }

  Position encode(initializer_list<Side> vec) const {
    return geom.encode(vec);
  }

  char encode_points(int points) const {
    return geom.encode_points(points);
  }

  void dump_similar(NodeLine line) const {
    trie.dump_similar(line);
  }

  bool has_symmetry(NodeLine line) const {
    return !trie.is_identity(line);
  }

  Zobrist get_zobrist(Position pos, Mark mark) const {
    return mark == Mark::X ? geom.zobrist_x()[pos] : geom.zobrist_o()[pos];
  }

 private:
  const Geometry<N, D> geom;
  const Symmetry<N, D> sym;
  const SymmeTrie<N, D> trie;
};

#endif
