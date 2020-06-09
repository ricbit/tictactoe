#ifndef SOLUTIONTREE_HH
#define SOLUTIONTREE_HH

#include <variant>
#include "boarddata.hh"

class SolutionTree {
  struct Child {
    Position pos;
    shared_pointer<Node> next;
  };
  struct Node {
    Mark value;
    int count;
    vector<Child> children;
  };
  shared_pointer<Node> root;
};

#endif
