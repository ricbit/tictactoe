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
      elevator[line].index = Line{line};
      elevator[line].floor = 0_mcount;
    }
    for (NodeP line = 1_np; line < line_size - 1; ++line) {
      elevator[line].left = NodeP{line - 1};
      elevator[line].right = NodeP{line + 1};
    }
    elevator[0_np].left = floor(0_mcount);
    elevator[0_np].right = 1_np;
    elevator[NodeP(line_size - 1)].left = NodeP{line_size - 2};
    elevator[NodeP(line_size - 1)].right = floor(0_mcount);
    for (MarkCount mark = 0_mcount; mark <= N; ++mark) {
      elevator[floor(mark)].index = line_size;
      elevator[floor(mark)].floor = mark;
    }
    elevator[floor(0_mcount)].left = NodeP{line_size - 1};
    elevator[floor(0_mcount)].right = 0_np;
    for (MarkCount mark = 1_mcount; mark <= N; ++mark) {
      elevator[floor(mark)].left = floor(mark);
      elevator[floor(mark)].right = floor(mark);
    }
  }

  NodeP floor(MarkCount mark) const {
    return NodeP{line_size + mark};
  }

  struct ElevatorElement {
    NodeP line;
    Elevator& instance;
    operator MarkCount() const {
      return instance.elevator[line].floor;
    }
    // O(1)
    MarkCount operator++() {
      return reattach_node(MarkCount{++instance.elevator[line].floor});
    }
    // O(1)
    MarkCount operator--() {
      return reattach_node(MarkCount{--instance.elevator[line].floor});
    }
    MarkCount reattach_node(MarkCount current) {
      auto& elevator = instance.elevator;
      NodeP floor = instance.floor(current);
      NodeP last = elevator[floor].left;
      auto& eline = elevator[line];
      elevator[eline.left].right = eline.right;
      elevator[eline.right].left = eline.left;
      eline.left = last;
      eline.right = floor;
      elevator[floor].left = line;
      elevator[last].right = line;
      return current;
    }
  };

  ElevatorElement operator[](Line line) {
    return ElevatorElement{NodeP{line}, *this};
  }

  struct Node {
    MarkCount floor;
    Line index;
    NodeP left, right;
  };

  struct Iterator {
    const Elevator& instance;
    NodeP node;
    bool operator!=(const Iterator& that) const {
      return node != that.node;
    }
    // O(1)
    Line operator*() const {
      return instance.elevator[node].index;
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

  ElevatorRange all(MarkCount mark) const {
    return ElevatorRange{*this, floor(mark)};
  }
 private:
  constexpr static Line line_size = BoardData<N, D>::line_size;
  sarray<NodeP, Node, line_size + N + 1> elevator;
};

#endif
