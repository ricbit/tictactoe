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

template<int N, int D>
class SolutionDag {
 public:
  SolutionDag(const BoardData<N, D>& data, NodeIndex max_nodes) : data(data) {
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
    if (has_chaining(state, child_turn)) {
      nodes.push_back(DagNode{state, child.parent, 0_cind, child_turn});
      childp = NodeP{&*nodes.rbegin()};
      return childp;
    }
    auto forcing_move = has_forcing_move(state, child_turn);
    if (forcing_move.has_value()) {
      nodes.push_back(DagNode{state, child.parent, 1_cind, child_turn});
      childp = NodeP{&*nodes.rbegin()};
      return childp;
    }
    ChildIndex children_size = static_cast<ChildIndex>(
        state.get_open_positions(to_mark(child_turn)).count());
    nodes.push_back(DagNode{state, child.parent, children_size, child_turn});
    childp = NodeP{&*nodes.rbegin()};
    return childp;
  }

  bool has_chaining(State<N, D>& state, Turn turn) {
    auto c = ChainingStrategy(state);
    auto pos = c.search(to_mark(turn));
    return pos.has_value();
  }

  optional<Position> has_forcing_move(State<N, D>& state, Turn turn) {
    auto c = ForcingMove<N ,D>(state);
    auto available = state.get_open_positions(to_mark(turn));
    auto pos = c.check(to_mark(turn), available);
    return pos.first;
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
    if (has_chaining(state, turn)) {
      return {};
    }
    auto forcing_move = has_forcing_move(state, turn);
    if (forcing_move.has_value()) {
      return {*forcing_move};
    }
    return state.get_open_positions(to_mark(get_turn(node))).get_bag();
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

}

#endif
