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

  NodeP operator=(const NodeP& other) {
    return NodeP{other.node};
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
 private:
  DagNode * const node;
};

struct Child {
  const NodeP parent;
  const ChildIndex index;
};

class DagNode {
 public:
  template<int N, int D>
  DagNode(const State<N, D>& state, const NodeP parent, ChildIndex children_size)
      : children(children_size) {
    if (!parent.empty()) {
      parents.push_back(parent);
    }
  }

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
    nodes.push_back(DagNode{initial, NodeP{nullptr}, children_size});
  }

  DagNode& get_node(const NodeP node) {    
    return *node;
  }

  const DagNode& get_node(const NodeP node) const {    
    return *node;
  }

  NodeP get_child(const Child child) {
    // Create the node if doesn't exist
    // AND
    // with the right number of children based on ChainingStrategy and ForcingStrategy
    // 
    auto childp = get_node(child.parent).children[child.index];
    if (!childp.empty()) {
      return childp;
    }
    auto state = get_state(child.parent);
    auto parent_positions = get_positions(child.parent);
    auto parent_turn = get_turn(child.parent);
    auto child_turn = flip(parent_turn);
    state.play(parent_positions[child.index], to_mark(parent_turn));
    ChildIndex children_size = static_cast<ChildIndex>(
        state.get_open_positions(to_mark(child_turn)).count());
    nodes.push_back(DagNode{state, child.parent, children_size});
    return NodeP{&*nodes.rbegin()};
  }

  NodeP get_root() {
    return NodeP{&nodes[0_nind]};
  }

  const auto& get_parents(const NodeP node) const {
    return get_node(node).parents;
  }

  const ChildIndex children_size(const NodeP node) const {
    return static_cast<ChildIndex>(get_node(node).children.size());
  }

  const bag<Position> get_positions(const NodeP node) const {
    State<N, D> state = get_state(node);    
    return state.get_open_positions(to_mark(get_turn(node))).get_bag();
  }

  bool has_parent(const NodeP node) const {
    return !get_parents(node).empty();
  }

  Turn get_turn(const NodeP node) const {
    return get_depth(node) % 2 == 1 ? Turn::X : Turn::O;
  }

  State<N, D> get_state(const NodeP node) const {
    return get_state(node, get_turn(node));
  }

  State<N, D> get_state(const NodeP node, Turn turn) const {
    if (!has_parent(node)) {
      return State<N, D>{data};
    }
    return State<N, D>{data};
  }

  void add_variation(const bag<Position>& variation) {
  }

 private:
  const BoardData<N, D>& data;
  svector<NodeIndex, DagNode> nodes;
  unordered_map<Zobrist, NodeP> zobrist_map;

  int get_depth(const NodeP node) const {
    int depth = 1;
    for (NodeP p = node; has_parent(p); p = get_parents(p)[0_pind]) {
      depth++;
    }
    return depth;
  }
};

}

#endif
