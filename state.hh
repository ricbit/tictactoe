#ifndef STATE_HH
#define STATE_HH

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
#include "tracking.hh"
#include "elevator.hh"

template<int N, int D>
class State {
 public:
  explicit State(const BoardData<N, D>& data) :
      data(data),
      board(Mark::empty),
      x_marks_on_line(0_mcount),
      o_marks_on_line(0_mcount),
      xor_table(data.xor_table()),
      current_accumulation(data.accumulation_points()),
      trie_node(0_node) {
    active_line.set();
  }

  constexpr static Position board_size = BoardData<N, D>::board_size;
  constexpr static Line line_size = BoardData<N, D>::line_size;

  Bitfield<N, D> get_open_positions(Mark mark) const {
    Bitfield<N, D> open_positions;
    Bitfield<N, D> checked;
    for (Position i : empty_cells) {
      if (!checked[i]) {
        open_positions.set(i);
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

  void print() const {
    data.print(data.board_size, [&](Position k) {
      return data.decode(k);
    }, [&](Position k) {
      return encode_position(board[k]);
    });
  }

  void print_last_position(Position pos) const {
    data.print(data.board_size, [&](Position k) {
      return data.decode(k);
    }, [&](Position k) {
      string color = pos == k ? "\x1b[33m"s : "\x1b[37m"s;
      return color + encode_position(board[k]);
    });
  }

  template<typename T>
  bool all_line(const T& line, Mark mark) const {
    return all_of(begin(line), end(line), [&](Position pos) {
      return board[pos] == mark;
    });
  }

  void print_winner() const {
    set<Position> winners;
    for (const auto& line : data.winning_lines()) {
      if (all_line(line, Mark::X) || all_line(line, Mark::O)) {
        copy(begin(line), end(line), inserter(winners, begin(winners)));
      }
    }
    data.print(data.board_size, [&](Position k) {
      return data.decode(k);
    }, [&](Position k) {
      string color = winners.find(k) != winners.end() ? "\x1b[31m"s : "\x1b[37m"s;
      return color + encode_position(board[k]);
    });
  }

  bool play(Position pos, Mark mark) {
    board[pos] = mark;
    empty_cells.remove(pos);
    trie_node = data.next(trie_node, pos);
    for (Line line : data.lines_through_position()[pos]) {
      auto& current = get_current(mark);
      current[line]++;
      xor_table[line] ^= pos;
      line_marks[line] += mark;
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
          if (current_accumulation[neigh] == 0 && empty_cells.check(neigh)) {
            empty_cells.remove(neigh);
          }
        }
      }
    }
    return false;
  }

  auto get_line_marks(MarkCount count, Mark mark) const {
    return line_marks.all(count, mark);
  }

  const Position get_xor_table(Line line) const {
    return xor_table[line];
  }

  const LineCount get_current_accumulation(Position pos) const {
    return current_accumulation[pos];
  };

  Mark get_board(Position pos) const {
    return board[pos];
  }

  void print_accumulation() {
    data.print(data.board_size, [&](Position k) {
      return data.decode(k);
    }, [&](Position k) {
      return data.encode_points(current_accumulation[k]);
    });
  }

 private:
  const BoardData<N, D>& data;
  sarray<Position, Mark, board_size> board;
  sarray<Line, MarkCount, line_size> x_marks_on_line, o_marks_on_line;
  sarray<Line, Position, line_size> xor_table;
  bitset<line_size> active_line;
  sarray<Position, LineCount, board_size> current_accumulation;
  NodeLine trie_node;
  TrackingList<N, D> empty_cells;
  Elevator<N, D> line_marks;

  auto& get_current(Mark mark) {
    return mark == Mark::X ? x_marks_on_line : o_marks_on_line;
  }

  char encode_position(Mark pos) const {
    return pos == Mark::X ? 'X'
         : pos == Mark::O ? 'O'
         : '.';
  }
};

#endif
