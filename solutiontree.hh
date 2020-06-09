#ifndef SOLUTIONTREE_HH
#define SOLUTIONTREE_HH

#include <variant>

#include "boarddata.hh"

class SolutionTree {
  struct Node;
  struct Child {
    Position pos;
    shared_ptr<Node> next;
  };
  struct Node {
    Mark value;
    int count;
    vector<Child> children;
  };
  shared_ptr<Node> root;
};

#endif
