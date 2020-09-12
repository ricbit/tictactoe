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

using NodeP = DagNode*;

struct Child {
  const NodeP parent;
  const ChildIndex index;
};

class DagNode {
 public:
  template<int N, int D>
  DagNode(const State<N, D>& state, const NodeP parent) {
  }
  NodeP get_child(const ChildIndex index) {
    return children[index];
  }
 private:
  svector<ParentIndex, NodeP> parent;
  svector<ChildIndex, NodeP> children;
};

template<int N, int D>
class SolutionDag {
 public:
  SolutionDag(const BoardData<N, D>& data, NodeIndex max_nodes) : data(data) {
    nodes.reserve(max_nodes);
    nodes.push_back(DagNode{State<N, D>{data}, nullptr});
  }
  DagNode& get_node(const NodeP node) const {
  }
  DagNode& get_child(const Child child) {
  }
  NodeP get_root() {
    return &nodes[0_nind];
  }
 private:
  const BoardData<N, D>& data;
  svector<NodeIndex, DagNode> nodes;
  unordered_map<Zobrist, NodeP> zobrist_map;
};

}

#endif
