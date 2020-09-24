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

  NodeP get_child(const Child child) {
    // Create the node if doesn't exist
    // AND
    // with the right number of children based on ChainingStrategy and ForcingStrategy
    // 
    auto& childp = get_node(child.parent).children[child.index];
    if (!childp.empty()) {
      return childp;
    }
    cout << "child " << child.parent.debug_int() << " index ";
    cout << static_cast<int>(child.index) << "\n";
    auto state = get_state(child.parent);
    auto parent_positions = get_positions(child.parent);
    auto parent_turn = get_turn(child.parent);
    auto child_turn = flip(parent_turn);
    state.play(parent_positions[child.index], to_mark(parent_turn));
    if (has_chaining(state, child_turn)) {
      nodes.push_back(DagNode{state, child.parent, 0_cind});
      auto new_childp = NodeP{&*nodes.rbegin()};
      return childp = new_childp;
    }
    ChildIndex children_size = static_cast<ChildIndex>(
        state.get_open_positions(to_mark(child_turn)).count());
    nodes.push_back(DagNode{state, child.parent, children_size});
    auto new_childp = NodeP{&*nodes.rbegin()};
    return childp = new_childp;
  }

  bool has_chaining(State<N, D>& state, Turn turn) {
    auto c = ChainingStrategy(state);
    auto pos = c.search(to_mark(turn));
    cout << "pos " << pos.has_value() <<"\n";
    return pos.has_value();
  }

  NodeP get_root() {
    return NodeP{&nodes[0_nind]};
  }

  const auto& get_parents(const NodeP node) {
    return get_node(node).parents;
  }

  const auto& get_children(const NodeP node) const {
    return get_node(node).children;
  }

  const ChildIndex children_size(const NodeP node) {
    return static_cast<ChildIndex>(get_node(node).children.size());
  }

  const bag<Position> get_positions(const NodeP node) {
    State<N, D> state = get_state(node);    
    return state.get_open_positions(to_mark(get_turn(node))).get_bag();
  }

  bool has_parent(const NodeP node) {
    return !get_parents(node).empty();
  }

  Turn get_turn(const NodeP node) {
    return get_depth(node) % 2 == 1 ? Turn::X : Turn::O;
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
    state.play(get_position(first_parent, node), to_mark(turn));
    return state;
  }

  // lfold rfold
  // reduce
  // [1,2,3,4,5]
  // A=Ab
  //  ^
  // [3,3,4,5]
  //  ^
  // [6,4,5]
  //  ^
  // [10,5]
  // [15]
  // [1,2,3,4,5]
  // A=bA
  //          ^
  // [1,2,3,9]
  //        ^
  // [1,2,12]
  // [1,14]
  // [15]

  Position get_position(NodeP parent, NodeP child) {
    auto open_positions = get_positions(parent);
    auto children = get_children(parent);
    cout << "child " << child.debug_int() << "\nparent \n";
    for (ChildIndex i = 0_cind; i < children.size(); i++) {
      cout << children[i].debug_int() << "\n";
    }
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
