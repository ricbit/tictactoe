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
      elevator[line].value = ElevatorValue{0_mcount, Line{line}};
    }
    for (NodeP line = 1_np; line < line_size - 1; ++line) {
      elevator[line].left = NodeP{line - 1};
      elevator[line].right = NodeP{line + 1};
    }
    elevator[0_np].left = floor(0_mcount);
    elevator[0_np].right = 1_np;
    elevator[NodeP(line_size - 1)].left = NodeP{line_size - 2};
    elevator[NodeP(line_size - 1)].right = floor(0_mcount);
    elevator[floor(0_mcount)].left = NodeP{line_size - 1};
    elevator[floor(0_mcount)].right = 0_np;
    for (MarkCount count = 1_mcount; count <= N; ++count) {
      elevator[floor(count)].left = floor(count);
      elevator[floor(count)].right = floor(count);
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
      return reattach_node(MarkCount{++get_floor(line)});
    }
    // O(1)
    MarkCount operator-=(Mark mark) {
      return reattach_node(MarkCount{--get_floor(line)});
    }
   private:
    MarkCount& get_floor(const NodeP line) const {
      return get<ElevatorValue>(instance.elevator[line].value).floor;
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

  ElevatorRange all(MarkCount count) const {
    return ElevatorRange{*this, floor(count)};
  }
 private:
  struct ElevatorValue {
    MarkCount floor;
    Line index;
  };
  struct FloorValue {
  };
  struct Node {
    variant<ElevatorValue, FloorValue> value;
    NodeP left, right;
  };

  NodeP floor(MarkCount count) const {
    return NodeP{line_size + count};
  }

  constexpr static Line line_size = BoardData<N, D>::line_size;
  sarray<NodeP, Node, line_size + N + 1> elevator;
};

#endif
