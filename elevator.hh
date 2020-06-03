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
    for (MarkCount mark = 0_mcount; mark <= N; ++mark) {
      floor[mark].index = line_size;
      floor[mark].floor = mark;
    }
    floor[0_mcount].left = &elevator[Line(line_size - 1)];
    floor[0_mcount].right = &elevator[Line(0)];
    for (MarkCount mark = 1_mcount; mark <= N; ++mark) {
      floor[mark].left = &floor[mark];
      floor[mark].right = &floor[mark];
    }
  }

  template<typename T>
  T* convert_pointer(const Elevator& other, const T* other_node) {
    if (other_node->index == line_size) {
      return &floor[other_node->floor];
    } else {
      return &elevator[Line(other_node->floor)];
    }
  }

  Elevator(const Elevator& other) {
    for (Line line = 0_line; line < line_size; ++line) {
      elevator[line].index = other.elevator[line].index;
      elevator[line].floor = other.elevator[line].floor;
      elevator[line].right = convert_pointer(other, other.elevator[line].right);
      elevator[line].left = convert_pointer(other, other.elevator[line].left);
    }
    for (MarkCount mark = 0_mcount; mark <= N; ++mark) {
      floor[mark].index = other.floor[mark].index;
      floor[mark].floor = other.floor[mark].floor;
      floor[mark].right = convert_pointer(other, other.floor[mark].right);
      floor[mark].left = convert_pointer(other, other.floor[mark].left);
    }    
  }

  struct ElevatorElement {
    Line line;
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
      auto& floor = instance.floor;
      elevator[line].left->right = elevator[line].right;
      elevator[line].right->left = elevator[line].left;
      elevator[line].left = &floor[current];
      elevator[line].right = floor[current].right;
      floor[current].right = &elevator[line];
      floor[current].left->right = &elevator[line];
      return current;
    }
  };

  ElevatorElement operator[](Line line) {
    return ElevatorElement{line, *this};
  }

  struct Node {
    MarkCount floor;
    Line index;
    Node *left, *right;
  };

  struct Iterator {
    Node *node;
    bool operator!=(const Iterator& that) {
      return node != that.node;
    }
    // O(1)
    Line operator*() {
      return node->index;
    }
    // O(1)
    Iterator& operator++() {
      node = node->right;
      return *this;
    }
  };

  struct ElevatorRange {
    Node *root;
    Iterator end() {
      return Iterator{root};
    }
    Iterator begin() {
      return Iterator{root->right};
    }
  };

  ElevatorRange all(MarkCount mark) {
    return ElevatorRange{&floor[mark]};
  }
 private:
  constexpr static Line line_size = BoardData<N, D>::line_size;
  sarray<Line, Node, line_size> elevator;
  sarray<MarkCount, Node, N + 1> floor;
};

#endif
