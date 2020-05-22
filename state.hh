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

template<int N, int D>
class State {
 public:
  explicit State(const BoardData<N, D>& data) :
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

  explicit State(const State<N, D>& state) :
      data(state.data),
      board(state.board),
      x_marks_on_line(state.x_marks_on_line), 
      o_marks_on_line(state.o_marks_on_line), 
      xor_table(state.xor_table),
      active_line(state.active_line),
      current_accumulation(state.current_accumulation),
      trie_node(state.trie_node),
      empty_cells(state.empty_cells),
      empty_it(state.empty_it.size(), empty_cells.end()) {
    for (auto it = begin(empty_cells); it != end(empty_cells); ++it) {
      empty_it[*it] = it;
    }
  }

  constexpr static Position board_size = BoardData<N, D>::board_size;
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

  Mark get_board(Position pos) const {
    return board[pos];
  }

 private:
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

  char encode_position(Mark pos) const {
    return pos == Mark::X ? 'X'
         : pos == Mark::O ? 'O'
         : '.';
  }
};

#endif
