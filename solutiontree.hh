#ifndef SOLUTIONTREE_HH
#define SOLUTIONTREE_HH

#include <variant>
#include <fstream>
#include "boarddata.hh"

class SolutionTree {
 public:
  enum class Reason {
    OUT_OF_NODES,
    ZOBRIST,
    DRAW,
    FORCING_MOVE,
    WIN,
    PRUNING,
    MINIMAX,
    CHAINING
  };
  struct Node {
    Node(Node *parent, int children_size, Zobrist zobrist)
        : value(BoardValue::UNKNOWN), count(1), parent(parent), zobrist(zobrist) {
      children.reserve(children_size);
    }
    Node(Node *parent, int children_size)
        : value(BoardValue::UNKNOWN), count(1), parent(parent), zobrist(Zobrist{0}) {
      children.reserve(children_size);
    }
    BoardValue value;
    int count;
    Reason reason;
    vector<pair<Position, unique_ptr<Node>>> children;
    Node *parent;
    Zobrist zobrist;
    Node *get_last_child() const {
      return children.rbegin()->second.get();
    }
    const BoardValue get_parent_value() const {
      return parent == nullptr ? BoardValue::O_WIN : parent->value;
    }
  };
  explicit SolutionTree(int board_size) : root(make_unique<Node>(nullptr, board_size, Zobrist{0})) {
  }
  Node *get_root() {
    return root.get();
  }
  const Node *get_root() const {
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
  void prune() {
    prune(root.get(), Mark::X);
  }
  int real_count() const {
    return real_count(root.get());
  }
 private:
  int real_count(Node *node) const {
    return accumulate(begin(node->children), end(node->children), 1,
        [&](auto acc, const auto& child) {
      return acc + real_count(child.second.get());
    });
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
      if ((node->value == BoardValue::O_WIN || node->value == BoardValue::DRAW)
          && node->children.size() != 1) {
        return false;
      }
    }
    return all_of(begin(node->children), end(node->children), [&](auto& child) {
      return validate(child.second.get(), flip(mark));
    });
  }
  void prune(Node *node, Mark mark) {
    if (mark == Mark::X) {
      if (node->value == BoardValue::X_WIN && node->children.size() > 1) {
        prune_children(node, BoardValue::X_WIN);
      }
    } else if (mark == Mark::O) {
      if ((node->value == BoardValue::O_WIN || node->value == BoardValue::DRAW)
          && node->children.size() > 1) {
        prune_children(node, max_child(node));
      }
    }
    for_each(begin(node->children), end(node->children), [&](auto& child) {
      prune(child.second.get(), flip(mark));
    });
  }

  void prune_children(Node *node, BoardValue goal) {
    remove_if(begin(node->children), end(node->children), [&](const auto& child) {
      return child.second->value != goal;
    });
    auto it = begin(node->children);
    if (it != end(node->children)) {
      ++it;
      node->children.erase(it, end(node->children));
    }
  }

  constexpr static auto compare_children = [](const auto& child1, const auto& child2) {
    return child1.second->value < child2.second->value;
  };

  BoardValue min_child(Node *node) const {
    auto min = min_element(begin(node->children), end(node->children), compare_children);
    return min->second->value;
  }

  BoardValue max_child(Node *node) const {
    auto max = max_element(begin(node->children), end(node->children), compare_children);
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
