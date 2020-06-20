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
#include "state.hh"
#include "solutiontree.hh"

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
class ForcingMove {
 public:
  explicit ForcingMove(const State<N, D>& state) : state(state) {
  }
  const State<N, D>& state;
  constexpr static Line line_size = BoardData<N, D>::line_size;

  optional<Position> find_forcing_move(
      Mark mark,
      const Bitfield<N, D>& open_positions) {
    for (Line line : state.get_line_marks(MarkCount{N - 1}, mark)) {
      Position trial = state.get_xor_table(line);
      if (open_positions[trial]) {
        return trial;
      }
    }
    // Can't return directly because of g++ bug.
    optional<Position> empty = {};
    return empty;
  }

  template<typename B>
  optional<Position> operator()(Mark mark, const B& open_positions) {
    return
        find_forcing_move(mark, open_positions) ||
        [&](){ return find_forcing_move(flip(mark), open_positions); };
  }

  template<typename B>
  pair<optional<Position>, Mark> check(Mark mark, const B& open_positions) {
    auto current = find_forcing_move(mark, open_positions);
    if (current.has_value()) {
      return make_pair(current, mark);
    }
    auto opponent = find_forcing_move(flip(mark), open_positions);
    if (opponent.has_value()) {
      return make_pair(opponent, flip(mark));
    }
    return {{}, Mark::empty};
  }
};

auto empty_printer = [](const auto& x){};

template<int N, int D, typename Print = decltype(empty_printer)>
class ChainingStrategy {
 public:
  explicit ChainingStrategy(const State<N, D>& state)
    : state(state) {
  }
  const State<N, D>& state;
  int visited = 0;
  constexpr static Line line_size = BoardData<N, D>::line_size;

  template<typename B>
  optional<Position> operator()(Mark mark, const B& open_positions) {
    return search(mark);
  }

  optional<Position> search(Mark mark) {
    return search_current(state, mark);
  }

  optional<Position> search_current(const State<N, D>& current, Mark mark) {
    visited++;
    Print()(current);
    for (Line line : current.get_line_marks(MarkCount{N - 1}, mark)) {
      return current.get_xor_table(line);
    }
    if (!current.empty(MarkCount{N - 1}, flip(mark))) {
      return {};
    }
    for (const auto& line : current.get_line_marks(MarkCount{N - 2}, mark)) {
      for (Position pos : current.get_line(line)) {
        if (current.get_board(pos) == Mark::empty) {
          State cloned(current);
          cloned.play(pos, mark);
          optional<Position> opponent = search_opponent(cloned, flip(mark));
          if (opponent.has_value()) {
            return pos;
          }
        }
      }
    }
    return {};
  }

  optional<Position> search_opponent(State<N, D>& current, Mark mark) {
    visited++;
    Print()(current);
    if (!current.empty(MarkCount{N - 1}, mark)) {
      return {};
    }
    if (!current.one(MarkCount{N - 1}, flip(mark))) {
      Position dummy = 0_pos;
      return dummy;
    }
    Line line = *current.get_line_marks(MarkCount{N - 1}, flip(mark)).begin();
    Position pos = current.get_xor_table(line);
    current.play(pos, mark);
    optional<Position> value = search_current(current, flip(mark));
    if (value.has_value()) {
      return pos;
    }
    return {};
  }
};

template<int N, int D>
class ForcingStrategy {
 public:
  explicit ForcingStrategy(
    const State<N, D>& state, const BoardData<N, D>& data) :
      state(state), data(data) {
  }
  const State<N, D>& state;
  const BoardData<N, D>& data;
  constexpr static Position board_size = BoardData<N, D>::board_size;

  optional<Position> find_forcing_move(Mark mark,
      const Bitfield<N, D>& open_positions) {
    for (Position pos : open_positions.all()) {
      for (const auto& [line_a, line_b] : data.crossings()[pos]) {
        if (state.check_line(line_a, MarkCount{N - 2}, mark) &&
            state.check_line(line_b, MarkCount{N - 2}, mark)) {
          return pos;
        }
      }
    }
    // Can't return directly because of g++ bug.
    optional<Position> empty = {};
    return empty;
  }

  template<typename B>
  optional<Position> operator()(Mark mark, const B& open_positions) {
    return
        find_forcing_move(mark, open_positions) ||
        [&](){ return find_forcing_move(flip(mark), open_positions); };
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
  constexpr static Position board_size = BoardData<N, D>::board_size;

  template<typename B>
  optional<Position> operator()(Mark mark, const B& open_positions) {
    auto open_pos = open_positions.all();
    int total = accumulate(begin(open_pos), end(open_pos), 0,
      [&](int a, auto pos) {
        return a + state.get_current_accumulation(pos);
      });
    uniform_int_distribution<int> random_position(0, total - 1);
    int chosen = random_position(generator);
    int previous = 0;
    for (Position pos : open_positions.all()) {
      int current = previous + state.get_current_accumulation(pos);
      if (chosen < current) {
        return pos;
      }
      previous = current;
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

template<Strategy A, Strategy B>
constexpr auto operator>>(A a, B b) {
  return Combiner<A, B>(a, b);
}

template<int N, int D>
class HeatMap {
 public:
  HeatMap(
    const State<N, D>& state,
    const BoardData<N, D>& data,
    default_random_engine& generator,
    int trials,
    bool print_board = false)
      : state(state), data(data), generator(generator),
        trials(trials), print_board(print_board) {
  }
  const State<N, D>& state;
  const BoardData<N, D>& data;
  default_random_engine& generator;
  int trials;
  bool print_board;
  constexpr static Line line_size = BoardData<N, D>::line_size;
  constexpr static Position board_size = BoardData<N, D>::board_size;

  template<typename B>
  optional<Position> operator()(Mark mark, const B& open_positions) {
    vector<Position> open = open_positions.get_vector();
    vector<int> score = get_scores(mark, open);
    if (print_board) {
      vector<int> norm = normalize_score(score);
      print(open, norm);
    }
    auto winner = max_element(begin(score), end(score));
    return open[distance(begin(score), winner)];
  }

  vector<int> get_scores(Mark mark, const vector<Position>& open) {
    Mark flipped = flip(mark);
    vector<int> score(open.size());
    transform(execution::par_unseq, begin(open), end(open), begin(score),
        [&](Position pos) {
      return monte_carlo(mark, flipped, pos);
    });
    return score;
  }

  vector<int> normalize_score(const vector<int>& score) {
    auto [vmin, vmax] = minmax_element(begin(score), end(score));
    double range = *vmax - *vmin;
    vector<int> norm(score.size());
    transform(begin(score), end(score), begin(norm), [&, vmin = vmin](int s) {
      if (range == 0.0) {
        return 9;
      } else {
        return static_cast<int>((s - *vmin) / range * 9.99);
      }
    });
    return norm;
  }

  int monte_carlo(Mark mark, Mark flipped, Position pos) {
    array<int, 3> win_counts = {0, 0, 0};
    for (int i = 0; i < trials; ++i) {
      State<N, D> cloned(state);
      cloned.play(pos, mark);
      auto s =
          ForcingMove<N, D>(cloned) >>
          ForcingStrategy<N, D>(cloned, data) >>
          BiasedRandom<N, D>(cloned, generator);
      GameEngine engine(generator, cloned, s);
      Mark winner = engine.play(flipped);
      win_counts[static_cast<int>(winner)]++;
    }
    return win_counts[static_cast<int>(mark)] -
           win_counts[static_cast<int>(flipped)];
  }

  void print(const vector<Position>& open, const vector<int>& norm) {
    data.print(board_size, [&](Position pos) {
      return data.decode(pos);
    }, [&](Position pos) {
      if (state.get_board(pos) != Mark::empty) {
        string color = "\x1b[32m"s;
        return color + (state.get_board(pos) == Mark::X ? 'X' : 'O');
      } else {
        string color = "\x1b[37m"s;
        auto it = find(begin(open), end(open), pos);
        if (it != end(open)) {
          return color + static_cast<char>(norm[distance(begin(open), it)] + '0');
        }
      }
      return "\x1b[30m."s;
    });
    cout << "\x1b[0m\n"s;
  }
};

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

  constexpr static Position board_size = BoardData<N, D>::board_size;

  template<typename T, typename U>
  Mark play(Mark start, T pre_observer, U post_observer) {
    Mark current_mark = start;
    while (true) {
      const auto& open_positions = state.get_open_positions(current_mark);
      if (open_positions.none()) {
        return Mark::empty;
      }
      pre_observer(open_positions);
      optional<Position> pos = strategy(current_mark, open_positions);
      if (pos.has_value()) {
        auto result = state.play(*pos, current_mark);
        post_observer(state, pos);
        if (result) {
          return current_mark;
        }
      } else {
        post_observer(state, pos);
      }
      current_mark = flip(current_mark);
    }
  }

  Mark play(Mark start) {
    return play(start, [](auto x){}, [](const auto& x, auto y){});
  }

  default_random_engine& generator;
  State<N, D>& state;
  S strategy;
};

#endif
