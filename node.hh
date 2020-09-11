
SEMANTIC_INDEX(ChildIndex, cind)
SEMANTIC_INDEX(ParentIndex, pind)
SEMANTIC_INDEX(NodeIndex, node)

class DagNode;
class SolutionDag;
using NodeP = DagNode*;

struct Child {
  const NodeP parent;
  const ChildIndex index;
}

template<int N, int D>
class DagNode {
 public:
  DagNode(const State<N, D>& state, const NodeP parent) {
  }
  NodeP get_child(const ChildIndex index) {
    return children[index];
  }
 private
  svector<ParentIndex, NodeP> parent;
  svector<ChildIndex, NodeP> children;
};

template<int N, int D>
class SolutionDag {
 public:
  SolutionDag(const BoardData<N, D>& data, int max_nodes) : data(data) {
    nodes.reserve(max_nodes);
    nodes.emplace_back(State<N, D>{data}, nullptr);
  }
  DagNode& get_node(const NodeP node) const {
 }
  DagNode& get_child(const Child child) {
  }
  const NodeP get_root() const {
    return &nodes[0];
  }
 private:
  const BoardData<N, D>& data;
  svector<NodeIndex, DagNode> nodes;
  unordered_map<Zobrist, NodeP> zobrist_map;
};

