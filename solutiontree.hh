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
    BoardValue valuex = BoardValue::UNKNOWN;
    int count = 1;
    Reason reasonx = Reason::UNKNOWN;
    vector<pair<Position, unique_ptr<Node>>> children;
    Node *parent;
    Zobrist zobrist;
    bool node_final = false;
    ProofNumber proof = 1_pn;
    ProofNumber disproof = 1_pn;
    Node *get_last_child() const {
      return children.rbegin()->second.get();
    }
    BoardValue get_value() const {
      return valuex;
    }
    void set_value(BoardValue value) {
      valuex = value;
    }
    Reason get_reason() const {
      return reasonx;
    }
    void set_reason(Reason reason) {
      reasonx = reason;
    }
    bool has_parent() const {
      return parent != nullptr;
    }
    const BoardValue get_parent_value() const {
      return has_parent() ? parent->get_value() : BoardValue::UNKNOWN;
    }
    bool is_final() const {
      return node_final;
    }
    bool is_parent_final() const {
      return parent == nullptr ? false : parent->is_final();
    }
    bool some_parent_final() const {
      for (Node *p = parent; p != nullptr; p = p->parent) {
        if (p->is_final()) {
          return true;
        }
      }
      return false;
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
      return len % 2 == 1 ? Turn::X : Turn::O;
    }
    template<int N, int D>
    State<N, D> rebuild_state(const BoardData<N, D>& data) const {
      Turn turn = flip(get_turn());
      State<N, D> state(data);
      rebuild_state(state, this, to_mark(turn));
      return state;
    }
    template<int N, int D>
    void rebuild_state(State<N, D>& state, const Node *p, Mark mark) const {
      if (p->parent != nullptr) {
        rebuild_state(state, p->parent, flip(mark));
        state.play(p->get_position(), mark);
      }
    }
    optional<BoardValue> best_child_X() const {
      return min_child();
    }
    optional<BoardValue> min_child() const {
      return extreme_child([](const auto& a, const auto&b) {
        return a < b;
      });
    }
    optional<BoardValue> best_child_O() const {
      auto highest_value = max_child();
      if (highest_value <= BoardValue::DRAW) {
        return highest_value;
      }
      bool has_draw_final = find_if(begin(children), end(children), [](const auto& child) {
        return child.second->is_final() && child.second->get_value() == BoardValue::DRAW;
      }) != end(children);
      bool has_o_win_final = find_if(begin(children), end(children), [](const auto& child) {
        return child.second->is_final() && child.second->get_value() == BoardValue::O_WIN;
      }) != end(children);
      if (has_draw_final && !has_o_win_final) {
        return BoardValue::DRAW;
      }
      return highest_value;
    }
    optional<BoardValue> max_child() const {
      return extreme_child([](const auto& a, const auto&b) {
        return a > b;
      });
    }
    double estimate_work() const {
      return estimate_work(0.0);
    }
   private:
    double estimate_work(double child_value) const {
      if (parent == nullptr) {
        return child_value;
      }
      double final_nodes = count_if(begin(parent->children), end(parent->children), [&](const auto& child) {
        return child.second->is_final() && child.second.get() != this;
      });
      double total_nodes = parent->children.size();
      return parent->estimate_work((final_nodes + child_value) / total_nodes);
    }
    template<typename T>
    optional<BoardValue> extreme_child(T comp) const {
      optional<BoardValue> ans;
      for (const auto& [pos, child] : children) {
        if (child->get_value() != BoardValue::UNKNOWN) {
          if (ans.has_value()) {
            ans = comp(*ans, child->get_value()) ? *ans : child->get_value();
          } else {
            ans = child->get_value();
          }
        }
      }
      return ans;
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
    if (!node->is_final()) {
      return false;
    }
    if (node->children.empty()) {
      return true;
    }
    if (mark == Mark::X) {
      if (node->min_child() != node->get_value()) {
        return false;
      }
      if (node->get_value() == BoardValue::X_WIN && node->children.size() != 1) {
        return false;
      }
    } else if (mark == Mark::O) {
      if (node->max_child() != node->get_value()) {
        return false;
      }
      if ((node->get_value() == BoardValue::O_WIN || node->get_value() == BoardValue::DRAW)
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
      if (node->get_value() == BoardValue::X_WIN && node->children.size() > 1) {
        prune_children(node, BoardValue::X_WIN);
      }
    } else if (mark == Mark::O) {
      if ((node->get_value() == BoardValue::O_WIN || node->get_value() == BoardValue::DRAW)
          && node->children.size() > 1) {
        prune_children(node, *node->best_child_O());
      }
    }
    for_each(begin(node->children), end(node->children), [&](auto& child) {
      prune(child.second.get(), flip(mark));
    });
  }

  void prune_children(Node *node, BoardValue goal) {
    [[maybe_unused]] auto r = remove_if(begin(node->children), end(node->children), [&](const auto& child) {
      return child.second->get_value() != goal || !child.second->is_final();
    });
    auto it = begin(node->children);
    if (it != end(node->children)) {
      ++it;
      node->children.erase(it, end(node->children));
    }
  }

  void dump_node(ofstream& ofs, const Node* node) const {
    ofs << static_cast<int>(node->get_value()) << " ";
    ofs << static_cast<int>(node->node_final) << " ";
    ofs << static_cast<int>(node->proof) << " ";
    ofs << static_cast<int>(node->disproof) << " ";
    ofs << node->count << " " << node->children.size() << " ";
    ofs << static_cast<int>(node->get_reason()) << " : ";
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
