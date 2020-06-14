#ifndef SOLUTIONTREE_HH
#define SOLUTIONTREE_HH

#include <variant>
#include "boarddata.hh"

class SolutionTree {
 public:
  struct Node {
    Node() : value(BoardValue::UNKNOWN), count(1) {
    }
    BoardValue value;
    int count;
    vector<pair<Position, unique_ptr<Node>>> children;
    Node *get_last_child() {
      return children.rbegin()->second.get();
    }
  };
  SolutionTree() : root(make_unique<Node>()) {
  }
  Node *get_root() {
    return root.get();
  }
 private:
  unique_ptr<Node> root;
};


#endif
