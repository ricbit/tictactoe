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
      if (min_child(node) != node->value) {
        return false;
      }
      if (node->value == BoardValue::X_WIN && node->children.size() != 1) {
        return false;
      }
    } else if (mark == Mark::O) {
      if (max_child(node) != node->value) {
        return false;
      }
      /*if ((node->value == BoardValue::O_WIN || node->value == BoardValue::DRAW)
          && node->children.size() != 1) {
        return false;
      }*/
    }
    return all_of(begin(node->children), end(node->children), [&](auto& child) {
      return validate(child.second.get(), flip(mark));
    });
  }
 private:
  constexpr static auto compare_children = [](const auto& child1, const auto& child2) {
    return child1.second->value < child2.second->value;
  };
  BoardValue min_child(Node *node) const {
    auto min = min_element(begin(node->children), end(node->children),
        compare_children);
    return min->second->value;
  }
  BoardValue max_child(Node *node) const {
    auto max = max_element(begin(node->children), end(node->children),
        compare_children);
    return max->second->value;
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
