#ifndef ELEVATOR_HH
#define ELEVATOR_HH

#include <variant>
#include "boarddata.hh"

SEMANTIC_INDEX(NodeP, np);

template<int N, int D>
class Elevator {
 public:
  Elevator() {
    for (NodeP line = 0_np; line < line_size; ++line) {
      elevator[line].value = ElevatorValue{Mark::empty, 0_mcount, Line{line}};
    }
    for (NodeP line = 1_np; line < line_size - 1; ++line) {
      elevator[line].left = NodeP{line - 1};
      elevator[line].right = NodeP{line + 1};
    }
    elevator[0_np].left = floor(0_mcount, Mark::empty);
    elevator[0_np].right = 1_np;
    elevator[NodeP(line_size - 1)].left = NodeP{line_size - 2};
    elevator[NodeP(line_size - 1)].right = floor(0_mcount, Mark::empty);
    elevator[floor(0_mcount, Mark::empty)].left = NodeP{line_size - 1};
    elevator[floor(0_mcount, Mark::empty)].right = 0_np;
    for (MarkCount count = 1_mcount; count <= N; ++count) {
      elevator[floor(count, Mark::empty)].left = floor(count, Mark::empty);
      elevator[floor(count, Mark::empty)].right = floor(count, Mark::empty);
    }
    for (Mark mark = Mark::X; mark <= Mark::both;) {
      for (MarkCount count = 0_mcount; count <= N; ++count) {
        elevator[floor(count, mark)].left = floor(count, mark);
        elevator[floor(count, mark)].right = floor(count, mark);
      }
      mark = static_cast<Mark>(1 + static_cast<int>(mark));
    }
  }

  struct ElevatorElement {
    NodeP line;
    Elevator& instance;
    operator MarkCount() const {
      return get_floor(line);
    }
    // O(1)
    MarkCount operator+=(Mark mark) {
      MarkCount prev = get_floor(line);
      MarkCount next = MarkCount{++get_floor(line)};
      return reattach_node(mark, prev, next);
    }
    // O(1)
    MarkCount operator-=(Mark mark) {
      MarkCount prev = get_floor(line);
      MarkCount next = MarkCount{--get_floor(line)};
      return reattach_node(mark, prev, next);
    }
   private:
    MarkCount& get_floor(const NodeP line) const {
      return get<ElevatorValue>(instance.elevator[line].value).floor;
    }
    MarkCount reattach_node(Mark mark, MarkCount prev, MarkCount next) {
      auto& elevator = instance.elevator;
      Mark& prev_mark = get<ElevatorValue>(instance.elevator[line].value).mark;
      Mark next_mark = static_cast<Mark>(
          static_cast<int>(prev_mark) | static_cast<int>(mark));
      NodeP next_floor = instance.floor(next, next_mark);
      NodeP last = elevator[next_floor].left;
      auto& eline = elevator[line];
      elevator[eline.right].left = eline.left;
      elevator[eline.left].right = eline.right;
      eline.left = last;
      eline.right = next_floor;
      prev_mark = next_mark;
      elevator[next_floor].left = line;
      elevator[last].right = line;
      return next;
    }
  };

  ElevatorElement operator[](Line line) {
    return ElevatorElement{NodeP{line}, *this};
  }

  void dump() const {
    cout << "\n----\n";
    for (int i = 0; auto node : elevator) {
      cout << "node " << i++ << " left " << node.left;
      cout << " right " << node.right << "\n";
    }
  }

  struct Iterator {
    const Elevator& instance;
    NodeP node;
    bool operator!=(const Iterator& that) const {
      return node != that.node;
    }
    // O(1)
    Line operator*() const {
      return get<ElevatorValue>(instance.elevator[node].value).index;
    }
    // O(1)
    Iterator& operator++() {
      node = instance.elevator[node].right;
      return *this;
    }
  };

  struct ElevatorRange {
    const Elevator& instance;
    const NodeP root;
    Iterator end() const {
      return Iterator{instance, root};
    }
    Iterator begin() const {
      return Iterator{instance, instance.elevator[root].right};
    }
  };

  ElevatorRange all(MarkCount count, Mark mark) const {
    return ElevatorRange{*this, floor(count, mark)};
  }

  bool check(Line line, MarkCount count, Mark mark) const {
    return elevator_value(line).floor == count &&
           elevator_value(line).mark == mark;
  }

 private:
  struct ElevatorValue {
    Mark mark;
    MarkCount floor;
    Line index;
  };
  struct FloorValue {
  };
  struct Node {
    variant<ElevatorValue, FloorValue> value;
    NodeP left, right;
  };

  NodeP floor(MarkCount count, Mark mark) const {
    return NodeP{line_size + static_cast<int>(mark) * (N + 1) + count};
  }

  const ElevatorValue& elevator_value(Line line) const {
    return get<ElevatorValue>(elevator[NodeP(line)].value);
  }

  constexpr static Line line_size = BoardData<N, D>::line_size;
  sarray<NodeP, Node, line_size + 4 * (N + 1)> elevator;
};

#endif
