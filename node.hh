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
  DagNode(const State<N, D>& state, const NodeP parent) {
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
 private:
  svector<ParentIndex, NodeP> parents;
  svector<ChildIndex, NodeP> children;
};

template<int N, int D>
class SolutionDag {
 public:
  SolutionDag(const BoardData<N, D>& data, NodeIndex max_nodes) : data(data) {
    nodes.reserve(max_nodes);
    nodes.push_back(DagNode{State<N, D>{data}, NodeP{nullptr}});
  }

  DagNode& get_node(const NodeP node) {
    return *node;
  }

  DagNode& get_child(const Child child) {
  }

  NodeP get_root() {
    return NodeP{&nodes[0_nind]};
  }
 private:
  const BoardData<N, D>& data;
  svector<NodeIndex, DagNode> nodes;
  unordered_map<Zobrist, NodeP> zobrist_map;
};

}

#endif
