#ifndef SOLUTIONTREE_HH
#define SOLUTIONTREE_HH

#include <variant>
#include <fstream>
#include "boarddata.hh"

class SolutionTree {
 public:
  enum class Reason {
    X_LINE,
    O_LINE,
    DRAW,
    X_CHAINING,
    O_CHAINING,
    UNKNOWN
  };
  struct Node {
    explicit Node(int children_size) : value(BoardValue::UNKNOWN), count(1) {
      children.reserve(children_size);
    }
    BoardValue value;
    int count;
    Reason reason;
    vector<pair<Position, unique_ptr<Node>>> children;
    Node *get_last_child() const {
      return children.rbegin()->second.get();
    }
  };
  explicit SolutionTree(int board_size) : root(make_unique<Node>(board_size)) {
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
  bool validate() const {
    return validate(root.get(), Mark::X);
  }
  bool validate(Node *node, Mark mark) const {
    if (node->children.empty()) {
      return true;
    }
    if (mark == Mark::X) {
      switch (node->value) {
        case BoardValue::X_WIN:
          if (!at_least_one(node, BoardValue::X_WIN)) {
            return false;
          }
          if (node->children.size() != 1) {
            return false;
          }
          break;
        case BoardValue::DRAW:
          if (!at_least_one(node, BoardValue::DRAW)) {
            return false;
          }
          if (at_least_one(node, BoardValue::X_WIN)) {
            return false;
          }
          break;
        case BoardValue::O_WIN:
          if (any_of(begin(node->children), end(node->children), [&](auto& child) {
            return child.second->value != BoardValue::O_WIN;
          })) {
            return false;
          }
          break;
        case BoardValue::UNKNOWN:
          if (find_if(begin(node->children), end(node->children), [&](auto& child) {
            return child.second->value == BoardValue::UNKNOWN;
          }) == end(node->children)) {
            return false;
          }
          if (find_if(begin(node->children), end(node->children), [&](auto& child) {
            return child.second->value == BoardValue::X_WIN;
          }) != end(node->children)) {
            return false;
          }
          break;
      }
    }
    return all_of(begin(node->children), end(node->children), [&](auto& child) {
      return validate(child.second.get(), flip(mark));
    });
  }
 private:
  bool at_least_one(Node *node, BoardValue value) const {
    return find_if(begin(node->children), end(node->children), [&](auto& child) {
      return child.second->value == value;
    }) != end(node->children);
  }
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
