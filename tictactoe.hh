#ifndef TICTACTOE_HH
#define TICTACTOE_HH

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
#include "boarddata.hh"

template<int N, int D>
class State {
 public:
  State(const BoardData<N, D>& data) :
      data(data),
      board(Mark::empty),
      x_marks_on_line(data.line_size),
      o_marks_on_line(data.line_size),
      xor_table(data.xor_table()),
      active_line(data.line_size, true),
      current_accumulation(data.accumulation_points()),
      trie_node(0_node),
      empty_cells(board_size) {
    iota(begin(empty_cells), end(empty_cells), 0_pos);
    for (auto it = begin(empty_cells); it != end(empty_cells); ++it) {
      empty_it.push_back(it);
    }
  }

  using Bitfield = typename BoardData<N, D>::Bitfield;

  const Bitfield& get_open_positions(Mark mark) {
    open_positions.reset();
    checked.reset();
    for (Position i : empty_cells) {
      if (!checked[i]) {
        open_positions[i] = true;
        checked |= data.mask(trie_node, i);
      }
    }
    return open_positions;
  }

  const auto& get_current(Mark mark) const {
    return mark == Mark::X ? x_marks_on_line : o_marks_on_line;
  }

  const auto& get_opponent(Mark mark) const {
    return mark == Mark::X ? o_marks_on_line : x_marks_on_line;
  }

  void print() {
    data.print(data.board_size, [&](Position k) {
      return data.decode(k);
    }, [&](Position k) {
      return encode_position(board[k]);
    });
  }

  bool play(Position pos, Mark mark) {
    board[pos] = mark;
    empty_cells.erase(empty_it[pos]);
    empty_it[pos] = end(empty_cells);
    trie_node = data.next(trie_node, pos);
    for (Line line : data.lines_through_position()[pos]) {
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
          Position neigh = data.winning_lines()[line][j];
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

  const Position get_xor_table(Line line) const {
    return xor_table[line];
  }

  const LineCount get_current_accumulation(Position pos) const {
    return current_accumulation[pos];
  };

 private:
  constexpr static Position board_size = BoardData<N, D>::board_size;
  const BoardData<N, D>& data;
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

  auto& get_current(Mark mark) {
    return mark == Mark::X ? x_marks_on_line : o_marks_on_line;
  }

  auto& get_opponent(Mark mark) {
    return mark == Mark::X ? o_marks_on_line : x_marks_on_line;
  }

  void print_accumulation() {
    data.print(data.board_size, [&](Position k) {
      return data.decode(k);
    }, [&](Position k) {
      return data.encode_points(current_accumulation[k]);
    });
  }

  char encode_position(Mark pos) {
    return pos == Mark::X ? 'X'
         : pos == Mark::O ? 'O'
         : '.';
  }
};

template<typename T, typename F>
optional<T> operator||(optional<T> first, F func) {
  if (first.has_value()) {
    return first;
  }
  return func();
}

template<typename T>
concept Strategy = requires (T x) {
  { x(Mark::X, bitset<125>()) } -> same_as<optional<Position>>;
};

template<int N, int D, Strategy S>
class GameEngine;

template<int N, int D>
class HeatMap { 
 public:
  HeatMap(
    const State<N, D>& state, 
    const Geometry<N, D>& geom,
    const Symmetry<N, D>& sym,
    const SymmeTrie<N, D>& trie,
    default_random_engine& generator)
      : state(state), geom(geom), sym(sym), trie(trie), generator(generator) {
  }
  const State<N, D>& state;
  const Geometry<N, D>& geom;
  const Symmetry<N, D>& sym;
  const SymmeTrie<N, D>& trie;
  default_random_engine& generator;
  using Bitfield = typename State<N, D>::Bitfield;
  constexpr static Line line_size = Geometry<N, D>::line_size;
  constexpr static Position board_size = Geometry<N, D>::board_size;

  template<typename B>
  optional<Position> operator()(Mark mark, const B& open_positions) {
    vector<Position> open;
    for (Position i = 0_pos; i < board_size; ++i) {
      if (open_positions[i]) {
        open.push_back(i);
      }
    }
    for_each(begin(open), end(open), [&](Position pos) {
      State<N, D> cloned(state);
      cloned.play(pos, mark);
      GameEngine engine(geom, sym, trie, generator, cloned);
      //engine.play
    });
    return {};
  }
};

template<int N, int D>
class ForcingMove { 
 public:
  explicit ForcingMove(const State<N, D>& state) : state(state) {
  }
  const State<N, D>& state;
  using Bitfield = typename State<N, D>::Bitfield;
  constexpr static Line line_size = Geometry<N, D>::line_size;

  optional<Position> find_forcing_move(
      const svector<Line, MarkCount>& current,
      const svector<Line, MarkCount>& opponent,
      const Bitfield& open_positions) {
    for (Line i = 0_line; i < line_size; ++i) {
      if (current[i] == N - 1 && opponent[i] == 0) {
        Position trial = state.get_xor_table(i);
        if (open_positions[trial]) {
          return trial;
        }
      }
    }
    return {};
  }

  template<typename B>
  optional<Position> operator()(Mark mark, const B& open_positions) {
    const auto& current = state.get_current(mark);
    const auto& opponent = state.get_opponent(mark);
    return         
        find_forcing_move(current, opponent, open_positions) ||
        [&](){ return find_forcing_move(opponent, current, open_positions); };
  }
};

template<int N, int D>
class BiasedRandom { 
 public:
  BiasedRandom(const State<N, D>& state, default_random_engine& generator)
      : state(state), generator(generator) {
  }
  const State<N, D>& state;
  default_random_engine& generator;
  using Bitfield = typename State<N, D>::Bitfield;
  constexpr static Position board_size = Geometry<N, D>::board_size;

  template<typename B>
  optional<Position> operator()(Mark mark, const B& open_positions) {
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
};

template<Strategy A, Strategy B>
class Combiner {
 public:
  Combiner(A a, B b) : a(a), b(b) {
  }
  A a;
  B b;
  template<typename T>
  optional<Position> operator()(Mark mark, const T& open_positions) {
    return a(mark, open_positions) || 
        [&](){ return b(mark, open_positions); };
  }
};

template<typename A, typename B>
constexpr auto make_combiner(A a, B b) {
  return Combiner<A, B>(a, b);
}

template<int N, int D, Strategy S>
class GameEngine {
 public:
  GameEngine(
    default_random_engine& generator,
    State<N, D>& state,
    S strategy) :
      generator(generator),
      state(state),
      strategy(strategy) {
  }

  using Bitfield = typename State<N, D>::Bitfield;
  constexpr static Position board_size = Geometry<N, D>::board_size;

  template<typename T>
  Mark play(Mark start, T observer) {
    Mark current_mark = start;
    while (true) {
      const auto& open_positions = state.get_open_positions(current_mark);
      if (open_positions.none()) {
        return Mark::empty;
      }
      observer(open_positions);
      optional<Position> pos = strategy(current_mark, open_positions);
      if (pos.has_value()) {
        auto result = state.play(*pos, current_mark);
        if (result) {
          return current_mark;
        }
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

  default_random_engine& generator;
  State<N, D>& state;
  S strategy;
};

#endif
