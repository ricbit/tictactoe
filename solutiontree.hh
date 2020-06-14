#ifndef SOLUTIONTREE_HH
#define SOLUTIONTREE_HH

#include <variant>
#include <fstream>
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
  template<int N, int D>
  void dump(const BoardData<N, D>& data, string filename) {
    ofstream ofs(filename);
    ofs << N << " " << D << "\n";
    dump_node(ofs, root.get());
  }
  void dump_node(ofstream& ofs, Node* node) {
    ofs << static_cast<int>(node->value) << " ";
    ofs << node->count << " " << node->children.size() << " : ";
    for (const auto& [pos, p] : node->children) {
      ofs << pos << "  ";
    }
    ofs << "\n";
    for (const auto& [pos, p] : node->children) {
      dump_node(ofs, p.get());
    }
  }
 private:
  unique_ptr<Node> root;
};


#endif
