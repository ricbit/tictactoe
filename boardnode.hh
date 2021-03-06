#ifndef BOARDNODE_HH
#define BOARDNODE_HH

#include "boarddata.hh"
#include "strategies.hh"

class DummyCout {
 public:
  template<typename T>
  const DummyCout& operator<<(const T& x) const {
    return *this;
  }
  template<typename T>
  const DummyCout& operator<<(T& x) const {
    return *this;
  }
  template<typename T>
  DummyCout& operator<<(const T& x) {
    return *this;
  }
  template<typename T>
  DummyCout& operator<<(T& x) {
    return *this;
  }
};

struct DefaultConfig {
  constexpr static NodeCount max_visited = 1'000'000_nc;
  constexpr static NodeCount max_created = 1'000'000_nc;
  DummyCout debug;
  bool should_prune = true;
  bool should_log_evolution = false;
};

enum class Reason {
#include "reason.hh"
};

ostream& operator<<(ostream& oss, const Reason& value) {
  switch (value) {
    case Reason::MINIMAX_EARLY:
      oss << "MINIMAX_EARLY"s;
      break;
    case Reason::MINIMAX_COMPLETE:
      oss << "MINIMAX_COMPLETE"s;
      break;
    case Reason::OUT_OF_NODES:
      oss << "OUT_OF_NODES"s;
      break;
    case Reason::ZOBRIST:
      oss << "ZOBRIST"s;
      break;
    case Reason::DRAW:
      oss << "DRAW"s;
      break;
    case Reason::FORCED_WIN:
      oss << "FORCED_WIN"s;
      break;
    case Reason::WIN:
      oss << "WIN"s;
      break;
    case Reason::PRUNING:
      oss << "PRUNING"s;
      break;
    case Reason::CHAINING:
      oss << "CHAINING"s;
      break;
    case Reason::UNKNOWN:
      oss << "UNKNOWN"s;
      break;
    }
  return oss;
}

template<int M>
class Node {
 public:
  static ProofNumber initial_proof(Turn turn, int children_size) {
    return turn == Turn::X ? 1_pn : ProofNumber{children_size};
  }
  static ProofNumber initial_disproof(Turn turn, int children_size) {
    return turn == Turn::X ? ProofNumber{children_size} : 1_pn;
  }
  Node(Node *parent_node, Turn turn, int children_size)
      : children_size(children_size), children(children_size, nullptr) {
    position.reserve(children_size);
    packed_values.parent = static_cast<signed>(distance(parent_node, this));
    packed_values.zobrist_first = static_cast<signed>(0);
    packed_values.zobrist_next = static_cast<signed>(0);
    packed_values.count = static_cast<unsigned>(0);
    packed_values.value = static_cast<uint8_t>(BoardValue::UNKNOWN);
    packed_values.reason = static_cast<uint8_t>(Reason::UNKNOWN);
    packed_values.is_final = static_cast<uint8_t>(false);
    packed_values.is_root = static_cast<uint8_t>(false);
    packed_values.is_eval = static_cast<uint8_t>(false);
    packed_values.proof = static_cast<unsigned>(initial_proof(turn, children_size));
    packed_values.disproof = static_cast<unsigned>(initial_disproof(turn, children_size));
  }
  constexpr static unsigned nodes_width = bit_width(static_cast<unsigned>(M));
  constexpr static unsigned pointer_width = 1 + nodes_width;
  constexpr static unsigned proof_width = 16;
  constexpr static ProofNumber INFTY = ProofNumber{(1u << proof_width) - 1};
  struct {
    uint8_t value : 2;
    uint8_t reason : 4;
    uint8_t is_final : 1;
    uint8_t is_root : 1;
    uint8_t is_eval : 1;
    unsigned count : nodes_width;
    signed parent : pointer_width;
    signed zobrist_first : pointer_width;
    signed zobrist_next : pointer_width;
    unsigned proof : proof_width;
    unsigned disproof : proof_width;
  } packed_values;
  unsigned children_size;
  bool children_built = false;
  float work = 0.0f;
  vector<uint8_t> position;
  vector<Node<M>*> children;

  bool has_position() const {
    return !position.empty();
  }

  void add_position(Position pos) {
    position.push_back(pos);
  }

  bool has_children() const {
    return children_built;
  }

  int get_position_index(Position pos) {
    for (int i = 0; i < static_cast<int>(position.size()); i++) {
      if (position[i] == pos) {
        return i;
      }
    }
    assert(false);
  }

  auto emplace_child(Position pos, Node *child) {
    children_built = true;
    children[get_position_index(pos)] = child;
    assert(children.size() <= children_size);
    return make_pair(pos, child);
  }

  const vector<pair<Position, Node*>> get_children() const {
    assert(children_built);
    vector<pair<Position, Node*>> copy_children;
    for (int i = 0; i < static_cast<int>(position.size()); i++) {
      Position pos = Position{position[i]};
      Node *child = children[i];
      if (child->get_reason() == Reason::PRUNING) {
        continue;
      }
      if (child->get_reason() == Reason::ZOBRIST) {
        copy_children.emplace_back(pos, child->get_zobrist_first());
      } else {
        copy_children.emplace_back(pos, child);
      }
    }
    return copy_children;
  }
  Node *get_child(int child) const {
    return children[child];
  }
  Node *get_zobrist_first() {
    Node *next = this;
    advance(next, -static_cast<signed>(packed_values.zobrist_first));
    return next;
  }
  Node *get_zobrist_next() {
    if (packed_values.zobrist_next == 0) {
      return nullptr;
    }
    Node *next = this;
    advance(next, -static_cast<signed>(packed_values.zobrist_next));
    return next;
  }
  void set_zobrist_first(Node *node) {
    packed_values.zobrist_first = static_cast<signed>(distance(node, this));
  }
  void set_zobrist_next(Node *node) {
    if (node == nullptr) {
      packed_values.zobrist_next = 0;
    } else {
      packed_values.zobrist_next = static_cast<signed>(distance(node, this));
    }
  }
  const Node *get_parent() const {
    const Node *next = this;
    advance(next, -static_cast<signed>(packed_values.parent));
    return next;
  }
  ProofNumber get_disproof() const {
    return static_cast<ProofNumber>(packed_values.disproof);
  }
  void set_disproof(ProofNumber disproof) {
    packed_values.disproof = static_cast<unsigned>(disproof);
  }
  ProofNumber get_proof() const {
    return static_cast<ProofNumber>(packed_values.proof);
  }
  void set_proof(ProofNumber proof) {
    packed_values.proof = static_cast<unsigned>(proof);
  }
  Node *get_parent() {
    Node *next = this;
    advance(next, -static_cast<signed>(packed_values.parent));
    return next;
  }
  NodeCount get_count() const {
    return static_cast<NodeCount>(packed_values.count);
  }
  NodeCount set_count(NodeCount count) {
    packed_values.count = static_cast<unsigned>(count);
    return count;
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
  bool is_eval() const {
    return packed_values.is_eval;
  }
  void set_is_eval(bool is_eval) {
    packed_values.is_eval = is_eval;
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

template<int N, int D, int M>
struct BoardNode {
  State<N, D> current_state;
  Turn turn;
  Node<M> *node;
};

template<int N, int D, int M>
struct Embryo {
  Position pos;
  LineCount accumulation_point;
  Node<M>* parent;
  Turn turn;
  int children_size;
  State<N, D> state;
  Node<M>** self;
  ProofNumber proof;
  ProofNumber disproof;
  Embryo(Position pos, LineCount accumulation_point, Node<M>* parent, Turn turn, int children_size,
         State<N, D> state, Node<M>** self)
      : pos(pos), accumulation_point(accumulation_point),
        parent(parent), turn(turn), children_size(children_size),
        state(state), self(self) {
    if (*self != nullptr) {
      proof = (*self)->get_proof();
      disproof = (*self)->get_disproof();
    } else {
      proof = Node<M>::initial_proof(turn, children_size);
      disproof = Node<M>::initial_disproof(turn, children_size);
    }
  }
};

template<int N, int D, typename Config = DefaultConfig>
class ChildrenBuilder {
 public:
  constexpr static NodeCount M = Config::max_created;
  constexpr static Config config = Config();

  bag<Embryo<N, D, M>> get_embryos(const BoardNode<N, D, M>& board_node) {
    auto& [current_state, turn, node] = board_node;
    auto open_positions = current_state.get_open_positions(to_mark(turn));
    vector<Position> sorted_positions;
    for (const auto pos : open_positions) {
      sorted_positions.push_back(pos);
    }
    sort(begin(sorted_positions), end(sorted_positions));
    bag<tuple<Position, LineCount, State<N, D>>> embryo_info =
        get_embryo_info(current_state, turn, open_positions, sorted_positions);
    bag<Embryo<N, D, M>> embryos;

    for (int i = 0; i < static_cast<int>(embryo_info.size()); i++) {
      auto& [position, current_accumulation, child_state] = embryo_info[i];
      auto children_size = child_state.get_open_positions(to_mark(flip(turn))).count();
      auto& embryo = embryos.emplace_back(
          position, current_accumulation, node, flip(turn), children_size, child_state, &node->children[i]);
      if (node->children[i] != nullptr) {
        embryo.proof = node->children[i]->get_proof();
        embryo.disproof = node->children[i]->get_disproof();
      }
    }
    if (!node->has_position()) {
      for (auto& embryo : embryos) {
        node->add_position(embryo.pos);
      }
    }
    return embryos;
  }

  template<typename S>
  auto build_children(S& solution, int& nodes_created, bag<Embryo<N, D, M>>& embryos) {
    bag<BoardNode<N, D, M>> children;
    for (int i = 0; i < static_cast<int>(embryos.size()); i++) {
      if (nodes_created == config.max_created) {
        return children;
      }
      nodes_created++;
      children.push_back(build_children(solution, embryos, i));
    }
    return children;
  }

  template<typename S>
  auto build_children(S& solution, bag<Embryo<N, D, M>>& embryos, int index) {
    auto& embryo = embryos[index];
    const auto& child = embryo.state;
    const pair<Position, Node<M>*>& child_node = embryo.parent->emplace_child(embryo.pos,
        solution.create_node(embryo.parent, embryo.turn, embryo.children_size));
    *embryo.self = child_node.second;
    return BoardNode<N, D, M>{child, embryo.turn, child_node.second};
  }

 private:
  template<typename B>
  auto get_embryo_info(const State<N, D>& current_state, Turn turn, B open_positions, vector<Position>& sorted) {
    bag<tuple<Position, LineCount, State<N, D>>> child_state;

    auto s = ForcingMove<N, D>(current_state);
    auto forcing = s.check(to_mark(turn), open_positions);
    /*current_state.print();
    cout << endl;*/
    if (forcing.first.has_value()) {
      assert(forcing.second != to_mark(turn));
      LineCount dummy_count = 0_lcount;
      child_state.clear();
      child_state.emplace_back(*forcing.first, dummy_count, current_state);
      State<N, D>& cloned = get<2>(*child_state.begin());
      bool game_ended = cloned.play(*forcing.first, to_mark(turn));
      assert(!game_ended);
    } else {
      child_state.reserve(sorted.size());
      for (auto position : sorted) {
        auto& last_child = child_state.emplace_back(
            position, current_state.get_current_accumulation(position), current_state);
        get<2>(last_child).play(position, to_mark(turn));
      }
    }
    return child_state;
  }
};

#endif
