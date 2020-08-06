#ifndef SOLUTIONTREE_HH
#define SOLUTIONTREE_HH

#include <variant>
#include <fstream>
#include "boarddata.hh"
#include "state.hh"

template<int M>
class SolutionTree {
 public:
  enum class Reason {
    #include "reason.hh"
  };

  class Node;

  class Children {
    //vector<pair<Position, Node*>> children;
   public:
    Node *first = nullptr;
    vector<uint8_t> position;
    auto emplace_back(Position pos, Node *child) {
      if (position.empty()) {
        first = child;
      }
      position.push_back(static_cast<uint8_t>(pos));
      return pair<Position, Node*>{pos, child};
    }
    void reserve(int size) {
      position.reserve(size);
    }
    bool empty() const {
      return position.empty();
    }
    auto size() const {
      return position.size();
    }
  };

  class Node {
   public:
    Node(Node *parent_node, Turn turn, int children_size) {
      assert(parent_node < this);
      childrenx.position.reserve(children_size);
      packed_values.parent = static_cast<unsigned>(distance(parent_node, this));
      packed_values.value = static_cast<uint8_t>(BoardValue::UNKNOWN);
      packed_values.reason = static_cast<uint8_t>(Reason::UNKNOWN);
      packed_values.is_final = static_cast<uint8_t>(false);
      packed_values.is_root = static_cast<uint8_t>(false);
      zobrist_next = nullptr;
      zobrist_first = this;
      proof = turn == Turn::X ? 1_pn : ProofNumber{children_size};
      disproof = turn == Turn::X ? ProofNumber{children_size} : 1_pn;
    }
    struct {
      uint8_t value : 2;
      uint8_t reason : 4;
      uint8_t is_final : 1;
      uint8_t is_root : 1;
      unsigned parent : bit_width(static_cast<unsigned>(M));
    } packed_values;
    float work = 0.0f;
    NodeCount count = 0_nc;
    Children childrenx;
    Node *zobrist_next;
    Node *zobrist_first;
    ProofNumber proof;
    ProofNumber disproof;

    auto emplace_child(Position pos, Node *parent) {
      return childrenx.emplace_back(pos, parent);
    }
    const vector<pair<Position, Node*>> get_children() const {
      vector<pair<Position, Node*>> copy_children;
      for (int i = 0; i < static_cast<int>(childrenx.position.size()); i++) {
          Position pos = Position{childrenx.position[i]};
          Node *child = childrenx.first + i;
          if (child->get_reason() == Reason::PRUNING) {
            continue;
          }
          if (child->get_reason() == Reason::ZOBRIST) {
            copy_children.emplace_back(pos, child->zobrist_first);
          } else {
            copy_children.emplace_back(pos, child);
          }
      }
      return copy_children;
    }
    Node *get_last_child() const {
      int last = childrenx.position.size() - 1;
      return childrenx.first + last;
    }
    const Node *get_parent() const {
      const Node *next = this;
      advance(next, -static_cast<signed>(packed_values.parent));
      return next;
    }
    Node *get_parent() {
      Node *next = this;
      advance(next, -static_cast<signed>(packed_values.parent));
      return next;
    }
    BoardValue get_value() const {
      return static_cast<BoardValue>(packed_values.value);
    }
    void set_value(BoardValue value) {
      packed_values.value = static_cast<uint8_t>(value);
    }
    Reason get_reason() const {
      return static_cast<Reason>(packed_values.reason);
    }
    void set_reason(Reason reason) {
      packed_values.reason = static_cast<uint8_t>(reason);
    }
    bool has_parent() const {
      return !is_root();
    }
    const BoardValue get_parent_value() const {
      return has_parent() ? get_parent()->get_value() : BoardValue::UNKNOWN;
    }
    bool is_root() const {
      return packed_values.is_root;
    }
    void set_is_root(bool is_root) {
      packed_values.is_root = is_root;
    }
    bool is_final() const {
      return packed_values.is_final;
    }
    void set_is_final(bool is_final) {
      packed_values.is_final = is_final;
    }
    bool is_parent_final() const {
      return has_parent() ? get_parent()->is_final() : false;
    }
    bool some_parent_final() const {
      for (const Node *p = this; p->has_parent();) {
        p = p->get_parent();
        if (p->is_final()) {
          return true;
        }
      }
      return false;
    }
    Position get_position() const {
      for (const auto& [pos, child] : get_parent()->get_children()) {
        if (child == this) {
          return pos;
        }
      }
      assert(false);
    }
    Turn get_turn() const {
      return get_depth() % 2 == 1 ? Turn::X : Turn::O;
    }
    int get_depth() const {
      int depth = 1;
      for (const Node *p = this; p->has_parent(); p = p->get_parent()) {
        depth++;
      }
      return depth;
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
      if (p->has_parent()) {
        rebuild_state(state, p->get_parent(), flip(mark));
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
      bool has_draw_final = has_final_children(BoardValue::DRAW);
      bool has_o_win_final = has_final_children(BoardValue::O_WIN);
      if (has_draw_final && !has_o_win_final) {
        return BoardValue::DRAW;
      }
      return highest_value;
    }
    bool has_final_children(BoardValue value) const {
      auto children = get_children();
      return find_if(begin(children), end(children), [&](const auto& child) {
        return child.second->is_final() && child.second->get_value() == value;
      }) != end(children);
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
      if (!has_parent()) {
        return child_value;
      }
      auto children = get_parent()->get_children();
      double final_nodes = count_if(begin(children), end(children), [&](const auto& child) {
        return child.second->is_final() && child.second != this;
      });
      double total_nodes = children.size();
      return get_parent()->estimate_work((final_nodes + child_value) / total_nodes);
    }

    template<typename T>
    optional<BoardValue> extreme_child(T comp) const {
      optional<BoardValue> ans;
      auto children = get_children();
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

  Node *get_root() {
    return root;
  }

  const Node *get_root() const {
    return root;
  }

  template<int N, int D>
  void dump(const BoardData<N, D>& data, string filename) const {
    ofstream ofs(filename);
    ofs << N << " " << D << "\n";
    dump_node(ofs, root);
  }

  bool validate() const {
    return validate(root, Mark::X);
  }

  void prune() {
    prune(root, Mark::X);
  }

  NodeCount real_count() const {
    return real_count(root);
  }

  void update_count() {
    update_count(root);
  }

  Node *create_node(Node *parent, Turn turn, int children_size) {
    return &nodes.emplace_back(parent, turn, children_size);
  }

  explicit SolutionTree(int board_size) {
    nodes.reserve(M);
    nodes.emplace_back(nullptr, Turn::X, 0);
    root = &nodes[0];
    root->set_is_root(true);
  }

 private:
  vector<Node> nodes;
  Node* root;

  NodeCount update_count(Node *node) {
    auto children = node->get_children();
    if (children.empty()) {
      return node->count = 1_nc;
    } else {
      return node->count = accumulate(begin(children), end(children), 1_nc,
          [&](auto acc, const auto& child) {
        return NodeCount{acc + update_count(child.second)};
      });
    }
  }

  NodeCount real_count(Node *node) const {
    auto children = node->get_children();
    return accumulate(begin(children), end(children), 1_nc,
        [&](auto acc, const auto& child) {
      return NodeCount{acc + real_count(child.second)};
    });
  }

  bool validate(Node *node, Mark mark) const {
    auto children = node->get_children();
    if (!node->is_final()) {
      cout << "Node is not final\n";
      return false;
    }
    if (children.empty()) {
      return true;
    }
    auto is_unknown = [](const auto &child) {
      return child.second->get_value() == BoardValue::UNKNOWN;
    };
    if (any_of(begin(children), end(children), is_unknown)) {
      return node->get_value() == BoardValue::UNKNOWN;
    }
    if (mark == Mark::X) {
      if (node->min_child() != node->get_value()) {
        cout << "X is not min\n";
        return false;
      }
      if (node->get_value() == BoardValue::X_WIN && children.size() != 1) {
        cout << "X_win is not unique\n";
        return false;
      }
    } else if (mark == Mark::O) {
      if (node->max_child() != node->get_value()) {
        cout << "O is not max\n";
        return false;
      }
      if ((node->get_value() == BoardValue::O_WIN || node->get_value() == BoardValue::DRAW)
          && children.size() != 1) {
        cout << node->get_value() << " is not unique\n";
        return false;
      }
    }
    return all_of(begin(children), end(children), [&](const auto& child) {
      return validate(child.second, flip(mark));
    });
  }

  void prune(Node *node, Mark mark) {
    auto children = node->get_children();
    if (mark == Mark::X) {
      if (node->get_value() == BoardValue::X_WIN && children.size() > 1) {
        prune_children(node, BoardValue::X_WIN);
      }
    } else if (mark == Mark::O) {
      if ((node->get_value() == BoardValue::O_WIN || node->get_value() == BoardValue::DRAW)
          && children.size() > 1) {
        prune_children(node, *node->best_child_O());
      }
    }
    for_each(begin(children), end(children), [&](const auto& child) {
      prune(child.second, flip(mark));
    });
  }

  void prune_children(Node *node, BoardValue goal) {
    for (int i = 0; i < static_cast<int>(node->childrenx.position.size()); i++) {
      Node *child = node->childrenx.first + i;
      if (child->get_value() != goal || !child->is_final()) {
        child->set_reason(Reason::PRUNING);
      }
    }
  }

  void dump_node(ofstream& ofs, const Node* node) const {
    auto children = node->get_children();
    ofs << static_cast<int>(node->get_value()) << " ";
    ofs << static_cast<int>(node->is_final()) << " ";
    ofs << static_cast<int>(node->proof) << " ";
    ofs << static_cast<int>(node->disproof) << " ";
    ofs << node->count << " " << children.size() << " ";
    ofs << static_cast<int>(node->get_reason()) << " : ";
    for (const auto& [pos, p] : children) {
      ofs << pos << "  ";
    }
    ofs << "\n";
    for (const auto& [pos, p] : children) {
      dump_node(ofs, p);
    }
  }
};


#endif
