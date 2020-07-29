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

  class Node;

  class Children {
    vector<pair<Position, Node*>> children;
   public:
    template<typename... Args>
    auto& emplace_back(Args&&... args) {
      return children.emplace_back(args...);
    }
    auto& operator[](int index) {
      return children[index];
    }
    void reserve(int size) {
      children.reserve(size);
    }
    template<typename A, typename B>
    void erase(A a, B b) {
      children.erase(a, b);
    }
    bool empty() const {
      return children.empty();
    }
    auto size() const {
      return children.size();
    }
    auto begin() const {
      return children.begin();
    }
    auto begin() {
      return children.begin();
    }
    auto rbegin() const {
      return children.rbegin();
    }
    auto rbegin() {
      return children.rbegin();
    }
    auto end() const {
      return children.end();
    }
    auto end() {
      return children.end();
    }
    auto rend() const {
      return children.rend();
    }
    auto rend() {
      return children.rend();
    }
   private:
  };

  struct Node {
    Node(Node *parent, int children_size, Zobrist zobrist) : parentx(parent), zobrist(zobrist) {
      init(children_size);
    }
    Node(Node *parent, int children_size) : parentx(parent), zobrist(Zobrist{0}) {
      init(children_size);
    }
    void init(int children_size) {
      children.reserve(children_size);
      packed_values.value = static_cast<uint8_t>(BoardValue::UNKNOWN);
      packed_values.reason = static_cast<uint8_t>(Reason::UNKNOWN);
      packed_values.is_final = static_cast<uint8_t>(false);
      packed_values.is_root = static_cast<uint8_t>(false);
    }
    struct {
      uint8_t value : 2;
      uint8_t reason : 4;
      uint8_t is_final : 1;
      uint8_t is_root : 1;
    } packed_values;
    int count = 1;
    Children children;
    Node *parentx; // 64 bits
    Zobrist zobrist;
    ProofNumber proof = 1_pn;
    ProofNumber disproof = 1_pn;
    Node *get_last_child() const {
      return children.rbegin()->second;
    }
    Node *get_parent() const {
      return parentx;
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
      for (const auto& [pos, child] : get_parent()->children) {
        if (child == this) {
          return pos;
        }
      }
      assert(false);
    }
    Turn get_turn() const {
      int len = 1;
      for (const Node *p = this; p->has_parent(); p = p->get_parent()) {
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
      if (!has_parent()) {
        return child_value;
      }
      double final_nodes = count_if(begin(get_parent()->children), end(get_parent()->children), [&](const auto& child) {
        return child.second->is_final() && child.second != this;
      });
      double total_nodes = get_parent()->children.size();
      return get_parent()->estimate_work((final_nodes + child_value) / total_nodes);
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
  int real_count() const {
    return real_count(root);
  }
  void update_count() {
    update_count(root);
  }
  SolutionTree(int board_size, int max_created)
      : max_nodes(max_created) {
    nodes.reserve(max_created);
    nodes.emplace_back(nullptr, 0);
    root = &nodes[0];
    root->set_is_root(true);
  }
  Node *create_node(Node *parent, int children_size) {
    nodes.emplace_back(parent, children_size);
    return &*nodes.rbegin();
  }
 private:
  int max_nodes;
  vector<Node> nodes;
  Node* root;
  int update_count(Node *node) {
    return node->count = accumulate(begin(node->children), end(node->children), 1,
        [&](auto acc, const auto& child) {
      return acc + update_count(child.second);
    });
  }
  int real_count(Node *node) const {
    return accumulate(begin(node->children), end(node->children), 1,
        [&](auto acc, const auto& child) {
      return acc + real_count(child.second);
    });
  }
  bool validate(Node *node, Mark mark) const {
    if (!node->is_final()) {
      return false;
    }
    if (node->children.empty()) {
      return true;
    }
    auto is_unknown = [](const auto &child) {
      return child.second->get_value() == BoardValue::UNKNOWN;
    };
    if (any_of(begin(node->children), end(node->children), is_unknown)) {
      return node->get_value() == BoardValue::UNKNOWN;
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
      return validate(child.second, flip(mark));
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
      prune(child.second, flip(mark));
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
    ofs << static_cast<int>(node->is_final()) << " ";
    ofs << static_cast<int>(node->proof) << " ";
    ofs << static_cast<int>(node->disproof) << " ";
    ofs << node->count << " " << node->children.size() << " ";
    ofs << static_cast<int>(node->get_reason()) << " : ";
    for (const auto& [pos, p] : node->children) {
      ofs << pos << "  ";
    }
    ofs << "\n";
    for (const auto& [pos, p] : node->children) {
      dump_node(ofs, p);
    }
  }
};


#endif
