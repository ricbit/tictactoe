#ifndef TRACKING_HH
#define TRACKING_HH

#include "boarddata.hh"

template<int N, int D>
class TrackingList {
 public:
  TrackingList() {
    for (Position i = 0_pos; i <= board_size; i++) {
      tracking_list[i] = make_pair(Position{i + 1}, Position{i - 1});
    }
    tracking_list[board_size].first = 0_pos;
    tracking_list[0_pos].second = board_size;
  }
  struct Iterator {
    const TrackingList& tlist;
    Position pos;
    bool operator!=(const Iterator& that) {
      return pos != that.pos;
    }
    Position operator*() {
      return pos;
    }
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
  void remove(Position pos) {
    tracking_list[tracking_list[pos].second].first = tracking_list[pos].first;
    tracking_list[tracking_list[pos].first].second = tracking_list[pos].second;
    tracking_list[pos].first = pos;
  }
  bool check(Position pos) const {
    return tracking_list[pos].first != pos;
  }
 private:
  constexpr static Position board_size = BoardData<N, D>::board_size;
  sarray<Position, pair<Position, Position>, board_size + 1> tracking_list;
};

#endif
