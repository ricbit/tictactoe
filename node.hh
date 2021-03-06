#ifndef NODE_HH
#define NODE_HH

#include "state.hh"

namespace node {

SEMANTIC_INDEX(ChildIndex, cind)
SEMANTIC_INDEX(ParentIndex, pind)
SEMANTIC_INDEX(NodeIndex, nind)

class DagNode;

template<int N, int D>
class SolutionDag;

class NodeP {
 public:
  NodeP() : node(nullptr) {
  }

  explicit NodeP(DagNode * node) : node(node) {
  }

  NodeP& operator=(const NodeP& other) {
    node = other.node;
    return *this;
  }

  bool operator==(const NodeP& other) const {
    return node == other.node;
  }

  bool operator!=(const NodeP& other) const {
    return !(*this == other);
  }

  DagNode& operator*() {
    return *node;
  }

  DagNode& operator*() const {
    return *node;
  }

  bool empty() const {
    return node == nullptr;
  }

  uint64_t debug_int() const {
    return reinterpret_cast<uint64_t>(node);
  }
 private:
  DagNode * node;
};

struct Child {
  const NodeP parent;
  const ChildIndex index;
};

class DagNode {
 public:
  template<int N, int D>
  DagNode(const State<N, D>& state, const NodeP parent, ChildIndex children_size, Turn turn)
      : turn(turn), children(children_size) {
    if (!parent.empty()) {
      parents.push_back(parent);
    }
  }

  Turn turn;
  Reason reason = Reason::UNKNOWN;
  BoardValue value = BoardValue::UNKNOWN;
  svector<ChildIndex, NodeP> children;
  svector<ParentIndex, NodeP> parents;
};

enum class PositionReason {
  CHAINING,
  FORCING,
  DRAW,
  MULTIPLE
};

class PositionEvaluator {
 public:
  pair<bag<Position>, PositionReason> get_positions(const auto& state, const Turn turn) const {
    if (has_chaining(state, turn)) {
      return {{}, PositionReason::CHAINING};
    }
    auto forcing_move = has_forcing_move(state, turn);
    if (forcing_move.has_value()) {
      return {{*forcing_move}, PositionReason::FORCING};
    }
    auto positions = state.get_open_positions(to_mark(turn)).get_bag();
    auto reason = positions.empty() ? PositionReason::DRAW : PositionReason::MULTIPLE;
    return {positions, reason};
  }

 private:
  bool has_chaining(const auto& state, Turn turn) const {
    auto c = ChainingStrategy(state);
    auto pos = c.search(to_mark(turn));
    return pos.has_value();
  }

  optional<Position> has_forcing_move(const auto& state, Turn turn) const {
    auto c = ForcingMove(state);
    auto available = state.get_open_positions(to_mark(turn));
    auto pos = c.check(to_mark(turn), available);
    return pos.first;
  }
};


template<int N, int D>
class SolutionDag {
 public:
  SolutionDag(const BoardData<N, D>& data, const PositionEvaluator& pos_eval, NodeIndex max_nodes)
      : data(data), pos_eval(pos_eval) {
    nodes.reserve(max_nodes);
    State<N, D> initial{data};
    ChildIndex children_size = static_cast<ChildIndex>(initial.get_open_positions(Mark::X).count());
    nodes.push_back(DagNode{initial, NodeP{nullptr}, children_size, Turn::X});
  }

  DagNode& get_node(const NodeP node) {
    return *node;
  }

  NodeP get_child(const Child child) {
    auto& childp = get_node(child.parent).children[child.index];
    if (!childp.empty()) {
      return childp;
    }
    auto state = get_state(child.parent);
    auto parent_positions = get_positions(child.parent);
    auto parent_turn = get_turn(child.parent);
    auto child_turn = flip(parent_turn);
    state.play(parent_positions[child.index], to_mark(parent_turn));
    auto [positions, reason] = pos_eval.get_positions(state, child_turn);
    nodes.push_back(DagNode{state, child.parent, static_cast<ChildIndex>(positions.size()), child_turn});
    childp = NodeP{&*nodes.rbegin()};
    get_node(childp).value = get_default_value(reason, child_turn);
    return childp;
  }

  BoardValue get_default_value(PositionReason reason, Turn turn) const {
    switch (reason) {
      case PositionReason::DRAW:
        return BoardValue::DRAW;
      case PositionReason::CHAINING:
        return turn == Turn::X ? BoardValue::X_WIN : BoardValue::O_WIN;
      default:
        return BoardValue::UNKNOWN;
    }
  }

  NodeP get_root() {
    return NodeP{&nodes[0_nind]};
  }

  const auto& get_parents(const NodeP node) {
    return get_node(node).parents;
  }

  const ChildIndex children_size(const NodeP node) {
    return static_cast<ChildIndex>(get_node(node).children.size());
  }

  const bag<Position> get_positions(const NodeP node) {
    State<N, D> state = get_state(node);
    Turn turn = get_turn(node);
    auto position_pair = pos_eval.get_positions(state, turn);
    return position_pair.first;
  }

  bool has_parent(const NodeP node) {
    return !get_parents(node).empty();
  }

  Turn get_turn(const NodeP node) {
    return get_node(node).turn;
  }

  BoardValue get_value(const NodeP node) {
    return get_node(node).value;
  }

  void set_value(const NodeP node, BoardValue value) {
    get_node(node).value = value;
  }

  State<N, D> get_state(const NodeP node) {
    return get_state(node, get_turn(node));
  }

  State<N, D> get_state(const NodeP node, Turn turn) {
    if (!has_parent(node)) {
      return State<N, D>{data};
    }
    auto first_parent = get_parents(node)[0_pind];
    auto state = get_state(first_parent, flip(turn));
    state.play(get_position(first_parent, node), to_mark(flip(turn)));
    return state;
  }

  Position get_position(NodeP parent, NodeP child) {
    auto open_positions = get_positions(parent);
    auto children = get_children(parent);
    auto it = find(begin(children), end(children), child);
    assert(it != end(children));
    auto index = distance(begin(children), it);
    return open_positions[index];
  }

  NodeP get_variation(const bag<Position>& variation) {
    auto current = get_root();
    for (auto variation_pos : variation) {
      auto open_positions = get_positions(current);
      auto it = find(begin(open_positions), end(open_positions), variation_pos);
      assert(it != end(open_positions));
      auto index = static_cast<ChildIndex>(distance(begin(open_positions), it));
      current = get_child(Child{current, index});
    }
    return current;
  }

 private:
  const BoardData<N, D>& data;
  const PositionEvaluator& pos_eval;
  svector<NodeIndex, DagNode> nodes;
  unordered_map<Zobrist, NodeP> zobrist_map;

  auto& get_children(const NodeP node) {
    return get_node(node).children;
  }

  int get_depth(const NodeP node) {
    int depth = 1;
    NodeP root = get_root();
    for (NodeP p = node; has_parent(p); p = get_parents(p)[0_pind]) {
      depth++;
    }
    return depth;
  }
};

template<int N, int D>
class MiniMax {
 public:
  MiniMax(SolutionDag<N, D>& solution) : solution(solution) {
  }

  //entra turn, node parent, [children]
  optional<BoardValue> update_parent_value(NodeP parent, Turn parent_turn, bag<NodeP> children) {
    auto valid_children = filter_unknowns(children);
    if (valid_children.empty()) {
      return {};
    }
    auto min_child = [&](const auto& a, const auto& b) {
      return solution.get_value(a) < solution.get_value(b);
    };
    auto max_child = [&](const auto& a, const auto& b) {
      return solution.get_value(a) > solution.get_value(b);
    };
    BoardValue new_value;
    if (parent_turn == Turn::X) {
      new_value = solution.get_value(*min_element(begin(valid_children), end(valid_children), min_child));
    } else {
      new_value = solution.get_value(*min_element(begin(valid_children), end(valid_children), max_child));
    }

    if (solution.get_value(parent) != new_value) {
      solution.set_value(parent, new_value);
      return {new_value};
    }
    return {};
  }
 private:
  SolutionDag<N, D>& solution;

  bag<NodeP> filter_unknowns(const bag<NodeP>& children) const {
    bag<NodeP> filtered;
    for (const auto& child : children) {
      if (solution.get_value(child) != BoardValue::UNKNOWN) {
        filtered.push_back(child);
      }
    }
    return filtered;
  }
};

}

#endif
