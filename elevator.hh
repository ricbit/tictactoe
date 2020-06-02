#ifndef ELEVATOR_HH
#define ELEVATOR_HH

#include <variant>
#include "boarddata.hh"

template<int N, int D>
class Elevator {
 public:
  Elevator() {
    for (Line line = 0_line; line < line_size; ++line) {
      elevator[line].index = line;
      elevator[line].floor = 0_mcount;
    }
    for (Line line = 1_line; line < line_size - 1; ++line) {
      elevator[line].left = &elevator[Line(line - 1)];
      elevator[line].right = &elevator[Line(line + 1)];
    }
    elevator[0_line].left = &floor[0_mcount];
    elevator[0_line].right = &elevator[1_line];
    elevator[Line(line_size - 1)].left = &elevator[Line(line_size - 2)];
    elevator[Line(line_size - 1)].right = &floor[0_mcount];
    floor[0_mcount].left = &elevator[Line(line_size - 1)];
    floor[0_mcount].right = &elevator[Line(0)];
    for (MarkCount side = 1_mcount; side <= N; ++side) {
      floor[side].left = &floor[side];
      floor[side].right = &floor[side];
    }
  }
  MarkCount operator[](Line line) const {
    auto x = elevator[line].floor;
    return MarkCount{x};
  }
  void inc(Line line) {
    MarkCount current = MarkCount{++elevator[line].floor};
    elevator[line].left->right = elevator[line].right;
    elevator[line].right->left = elevator[line].left;
    elevator[line].left = &floor[current];
    elevator[line].right = floor[current].right;
    floor[current].right = &elevator[line];
    floor[current].left->right = &elevator[line];
    
  }
  /*struct Iterator {
    const TrackingList& tlist;
    Position pos;
    bool operator!=(const Iterator& that) {
      return pos != that.pos;
    }
    // O(1)
    Position operator*() {
      return pos;
    }
    // O(1)
    Iterator& operator++() {
      pos = tlist.tracking_list[pos].first;
      return *this;
    }
  };
  Iterator end() {
    return Iterator{*this, board_size};
  }
  Iterator begin() {
    return Iterator{*this, tracking_list[board_size].first};
  }
  Iterator end() const {
    return Iterator{*this, board_size};
  }
  Iterator begin() const {
    return Iterator{*this, tracking_list[board_size].first};
  }
  // O(1)
  void remove(Position pos) {
    tracking_list[tracking_list[pos].second].first = tracking_list[pos].first;
    tracking_list[tracking_list[pos].first].second = tracking_list[pos].second;
    tracking_list[pos].first = pos;
  }
  // O(1)
  bool check(Position pos) const {
    return tracking_list[pos].first != pos;
  }*/
 private:
  struct Node {    
    MarkCount floor;
    Line index;
    Node *left, *right;
  };
  constexpr static int line_size = BoardData<N, D>::line_size;
  sarray<Line, Node, line_size> elevator;
  sarray<MarkCount, Node, N + 1> floor;
};

#endif
