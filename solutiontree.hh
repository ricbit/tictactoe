#ifndef SOLUTIONTREE_HH
#define SOLUTIONTREE_HH

#include <variant>
#include <fstream>
#include "boarddata.hh"

class SolutionTree {
 public:
  struct Node {
    Node(int children_size) : value(BoardValue::UNKNOWN), count(1) {
      children.reserve(children_size);
    }
    BoardValue value;
    int count;
    vector<pair<Position, unique_ptr<Node>>> children;
    Node *get_last_child() const {
      return children.rbegin()->second.get();
    }
  };
  SolutionTree(int board_size) : root(make_unique<Node>(board_size)) {
  }
  Node *get_root() {
    return root.get();
  }
  template<int N, int D>
  void dump(const BoardData<N, D>& data, string filename) const {
    ofstream ofs(filename);
    ofs << N << " " << D << "\n";
    dump_node(ofs, root.get());
  }
 private:
  void dump_node(ofstream& ofs, const Node* node) const {
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
  unique_ptr<Node> root;
};


#endif
