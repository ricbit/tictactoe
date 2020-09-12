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

  explicit NodeP(DagNode *node) : node(node) {
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

  NodeP get_child(const ChildIndex index) {
    return children[index];
  }

  const auto get_parents() const {
    return parents;
  }

  const ChildIndex children_size() const {
    return static_cast<ChildIndex>(children.size());
  }

 private:
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

  DagNode& get_child(const Child child) {
  }

  NodeP get_root() {
    return NodeP{&nodes[0_nind]};
  }

  const auto get_parents(const NodeP node) const {
    return (*node).get_parents();
  }

  const ChildIndex children_size(const NodeP node) const {
    return static_cast<ChildIndex>((*node).children_size());
  }

  const bag<Position> get_positions(const NodeP node) const {
    return {};
  }

 private:
  const BoardData<N, D>& data;
  svector<NodeIndex, DagNode> nodes;
  unordered_map<Zobrist, NodeP> zobrist_map;
};

}

#endif
