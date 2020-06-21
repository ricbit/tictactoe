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
      xor_table(data.xor_table()),
      current_accumulation(data.accumulation_points()),
      trie_node(0_node),
      zobrist(0) {
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

  void print() const {
    data.print(data.board_size, [&](Position k) {
      return data.decode(k);
    }, [&](Position k) {
      return encode_position(board[k]);
    });
  }

  void print_bitfield(const Bitfield<N, D>& bitfield, string marker) const {
    data.print(data.board_size, [&](Position k) {
      return data.decode(k);
    }, [&](Position k) {
      return bitfield[k] ? marker : "."s;
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

  bool play(initializer_list<Side> pos, Mark mark) {
    return play(data.encode(pos), mark);
  }

  bool play(Position pos, Mark mark) {
    board[pos] = mark;
    zobrist ^= data.get_zobrist(pos, mark);
    empty_cells.remove(pos);
    trie_node = data.next(trie_node, pos);
    for (Line line : data.lines_through_position()[pos]) {
      xor_table[line] ^= pos;
      Mark old_mark = line_marks.get_mark(line);
      MarkCount count = (line_marks[line] += mark);
      Mark new_mark = line_marks.get_mark(line);
      if (count == N && new_mark != Mark::both) {
        return true;
      }
      if (old_mark != new_mark && new_mark == Mark::both) {
        for (Position neigh : data.winning_lines()[line]) {
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

  bool has_symmetry() const {
    return data.has_symmetry(trie_node);
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

  void print_empty_cells() {
    data.print(data.board_size, [&](Position k) {
      return data.decode(k);
    }, [&](Position k) {
      return empty_cells.check(k) ? "E" : ".";
    });
  }

  bool check_line(Line line, MarkCount count, Mark mark) const {
    return line_marks.check(line, count, mark);
  }

  bool empty(MarkCount count, Mark mark) const {
    return line_marks.empty(count, mark);
  }

  bool one(MarkCount count, Mark mark) const {
    return line_marks.one(count, mark);
  }

  auto get_line(Line line) const {
    return data.winning_lines()[line];
  }

 private:
  const BoardData<N, D>& data;
  sarray<Position, Mark, board_size> board;
  sarray<Line, Position, line_size> xor_table;
  sarray<Position, LineCount, board_size> current_accumulation;
  NodeLine trie_node;
  TrackingList<N, D> empty_cells;
  Elevator<N, D> line_marks;
  Zobrist zobrist;

  char encode_position(Mark pos) const {
    return pos == Mark::X ? 'X'
         : pos == Mark::O ? 'O'
         : '.';
  }
};

#endif
