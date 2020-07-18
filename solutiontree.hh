#ifndef SOLUTIONTREE_HH
#define SOLUTIONTREE_HH

#include <variant>
#include <fstream>
#include "boarddata.hh"
#include "state.hh"

class SolutionTree {
 public:
  enum class Reason {
    #include "reason.hh"
  };
  struct Node {
    Node(Node *parent, int children_size, Zobrist zobrist) : parent(parent), zobrist(zobrist) {
      children.reserve(children_size);
    }
    Node(Node *parent, int children_size) : parent(parent), zobrist(Zobrist{0}) {
      children.reserve(children_size);
    }
    BoardValue value = BoardValue::UNKNOWN;
    int count = 1;
    Reason reason = Reason::UNKNOWN;
    vector<pair<Position, unique_ptr<Node>>> children;
    Node *parent;
    Zobrist zobrist;
    Node *get_last_child() const {
      return children.rbegin()->second.get();
    }
    bool has_parent() const {
      return parent == nullptr;
    }
    const BoardValue get_parent_value() const {
      return has_parent() ? BoardValue::O_WIN : parent->value;
    }
    bool is_final() const {
      return reason != Reason::MINIMAX_COMPLETE && reason != Reason::UNKNOWN;
    }
    bool is_parent_final() const {
      return parent == nullptr ? false : parent->is_final();
    }
    Position get_position() const {
      for (const auto& [pos, child] : parent->children) {
        if (child.get() == this) {
          return pos;
        }
      }
      assert(false);
    }
    Turn get_turn() const {
      int len = 0;
      for (const Node *p = this; p != nullptr; p = p->parent) {
        len++;
      }
      return len % 2 == 1 ? Turn::O : Turn::X;
    }
    template<int N, int D>
    State<N, D> rebuild_state(const BoardData<N, D>& data) const {
      Turn turn = get_turn();
      State<N, D> state(data);
      for (const Node *p = this; p->parent != nullptr; p = p->parent) {
        state.play(p->get_position(), to_mark(turn));
        turn = flip(turn);
      }
      return state;
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
  void update_count() {
    update_count(root.get());
  }
 private:
  int update_count(Node *node) {
    return node->count = accumulate(begin(node->children), end(node->children), 1,
        [&](auto acc, const auto& child) {
      return acc + update_count(child.second.get());
    });
  }
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
    auto filtered = filter_unknown(node->children);
    if (filtered.empty()) {
      return node->value == BoardValue::UNKNOWN;
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

  vector<BoardValue> filter_unknown(const vector<pair<Position, unique_ptr<Node>>>& children) const {
    vector<BoardValue> filtered;
    for (const auto& child : children) {
      if (child.second->value != BoardValue::UNKNOWN) {
        filtered.push_back(child.second->value);
      }
    }
    return filtered;
  }

  BoardValue min_child(Node *node) const {
    auto filtered = filter_unknown(node->children);
    return *min_element(begin(filtered), end(filtered));
  }

  BoardValue max_child(Node *node) const {
    auto filtered = filter_unknown(node->children);
    return *max_element(begin(filtered), end(filtered));
  }

  void dump_node(ofstream& ofs, const Node* node) const {
    ofs << static_cast<int>(node->value) << " ";
    ofs << node->count << " " << node->children.size() << " ";
    ofs << static_cast<int>(node->reason) << " : ";
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
