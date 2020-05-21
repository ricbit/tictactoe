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
    default_random_engine& generator)
      : state(state), data(data), generator(generator) {
  }
  const State<N, D>& state;
  const BoardData<N, D>& data;
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
    Mark flipped = flip(mark);
    vector<int> score(open.size());
    transform(execution::par_unseq, begin(open), end(open), begin(score),
        [&](Position pos) {
      vector<int> win_counts(3);
      int trials = 100;
      for (int i = 0; i < trials; ++i) {
        State<N, D> cloned(state);
        cloned.play(pos, mark);
        auto s = ForcingMove<N, D>(cloned) >> BiasedRandom<N, D>(cloned, generator);
        GameEngine engine(generator, cloned, s);
        Mark winner = engine.play(flipped, [](auto obs){});
        win_counts[static_cast<int>(winner)]++;
      }
      return win_counts[static_cast<int>(mark)] -
             win_counts[static_cast<int>(flipped)];
    });
    auto [vmin, vmax] = minmax_element(begin(score), end(score));
    double range = *vmax - *vmin;
    vector<int> norm(score.size());
    transform(begin(score), end(score), begin(norm), [&](int s) {
      if (range == 0.0) {
        return 9;
      } else {
        return static_cast<int>((s - *vmin) / range * 9.99);
      }
    });
    print(open, norm);
    auto winner = max_element(begin(score), end(score));
    return open[distance(begin(score), winner)];
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

  default_random_engine& generator;
  State<N, D>& state;
  S strategy;
};

#endif
