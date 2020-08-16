#ifndef SOLUTIONTREE_HH
#define SOLUTIONTREE_HH

#include <variant>
#include <fstream>
#include "boarddata.hh"
#include "state.hh"
#include "boardnode.hh"

template<int N, int D, int M>
class DotDumper {
 public:
  DotDumper(const Node<M>* root, const BoardData<N, D>& data, string filename)
      : ofs(filename), data(data), root(root) {
  }
  void dump() {
    collect_names(root);
    ofs << "digraph {\n";
    for (const auto& [node, node_name] : name) {
      ofs << "N" << node_name << " " << to_html(node) << " ;\n";
    }
    draw_edges(root);
    ofs << "}";
  }
 private:
  string to_html(const Node<M>* node) {
    ostringstream oss;
    oss << "[label=\"" << node->get_value() << "\n" << node->get_reason() << "\n";
    oss << " p " << node->get_proof() << " d " << node->get_disproof();
    oss << "\" ";
    if (node->is_final()) {
      oss << "color=red";
    }
    oss << " ]";
    return oss.str();
  }
  void draw_edges(const Node<M>* node) {
    if (node->has_children()) {
      for (const auto& [pos, child] : node->get_children()) {
        ofs << "N" << name[node] << " -> N" << name[child] << ";\n";
        draw_edges(child);
      }
    } else {
      ofs << "N" << name[node] << " -> X" << name[node] << ";\n";
      ofs << "X" << name[node] << " [shape=box];\n";
    }
  }
  void collect_names(const Node<M>* node) {
    name[node] = current++;
    if (node->has_children()) {
      for (const auto& child : node->get_children()) {
        collect_names(child.second);
      }
    }
  }
  ofstream ofs;
  const BoardData<N, D>& data;
  const Node<M>* root;
  int current = 0;
  unordered_map<const Node<M>*, int> name;
};

template<int M>
class SolutionTree {
 public:
  Node<M> *get_root() {
    return root;
  }

  const Node<M> *get_root() const {
    return root;
  }

  template<int N, int D>
  void dump(const BoardData<N, D>& data, string filename) const {
    ofstream ofs(filename);
    ofs << N << " " << D << "\n";
    dump_node(ofs, root);
  }

  template<int N, int D>
  void dump_dot(const BoardData<N, D>& data, string filename) const {
    DotDumper dd(root, data, filename);
    dd.dump();
  }

  bool validate() const {
    return validate(root, Mark::X);
  }

  void prune() {
    prune(root, Mark::X);
  }

  NodeCount real_count() const {
    return real_count(root);
  }

  void update_count() {
    update_count(root);
  }

  Node<M> *create_node(Node<M> *parent, Turn turn, int children_size) {
    return &nodes.emplace_back(parent, turn, children_size);
  }

  explicit SolutionTree(int board_size) {
    nodes.reserve(M);
    nodes.emplace_back(nullptr, Turn::X, 0);
    root = &nodes[0];
    root->set_is_root(true);
  }

 private:
  vector<Node<M>> nodes;
  Node<M>* root;

  NodeCount update_count(Node<M> *node) {
    if (!node->has_children()) {
      return node->set_count(1_nc);
    } else {
      auto children = node->get_children();
      return node->set_count(accumulate(begin(children), end(children), 1_nc,
          [&](auto acc, const auto& child) {
        return NodeCount{acc + update_count(child.second)};
      }));
    }
  }

  NodeCount real_count(Node<M> *node) const {
    if (!node->has_children()) {
      return 1_nc;
    }
    auto children = node->get_children();
    return accumulate(begin(children), end(children), 1_nc,
        [&](auto acc, const auto& child) {
      return NodeCount{acc + real_count(child.second)};
    });
  }

  bool validate(Node<M> *node, Mark mark) const {
    if (!node->is_final()) {
      cout << "Node<M> is not final\n";
      return false;
    }
    if (!node->has_children()) {
      return true;
    }
    auto is_unknown = [](const auto &child) {
      return child.second->get_value() == BoardValue::UNKNOWN;
    };
    auto children = node->get_children();
    if (any_of(begin(children), end(children), is_unknown)) {
      return node->get_value() == BoardValue::UNKNOWN;
    }
    if (mark == Mark::X) {
      if (node->min_child() != node->get_value()) {
        cout << "X is not min\n";
        return false;
      }
      if (node->get_value() == BoardValue::X_WIN && children.size() != 1) {
        cout << "X_win is not unique\n";
        return false;
      }
    } else if (mark == Mark::O) {
      if (node->max_child() != node->get_value()) {
        cout << "O is not max\n";
        return false;
      }
      if ((node->get_value() == BoardValue::O_WIN || node->get_value() == BoardValue::DRAW)
          && children.size() != 1) {
        cout << node->get_value() << " is not unique\n";
        return false;
      }
    }
    return all_of(begin(children), end(children), [&](const auto& child) {
      return validate(child.second, flip(mark));
    });
  }

  void prune(Node<M> *node, Mark mark) {
    if (!node->has_children()) {
      return;
    }
    auto children = node->get_children();
    if (mark == Mark::X) {
      if (node->get_value() == BoardValue::X_WIN && children.size() > 1) {
        prune_children(node, BoardValue::X_WIN);
      }
    } else if (mark == Mark::O) {
      if ((node->get_value() == BoardValue::O_WIN || node->get_value() == BoardValue::DRAW)
          && children.size() > 1) {
        prune_children(node, *node->best_child_O());
      }
    }
    for_each(begin(children), end(children), [&](const auto& child) {
      prune(child.second, flip(mark));
    });
  }

  void prune_children(Node<M> *node, BoardValue goal) {
    for (int i = 0; i < static_cast<int>(node->childrenx.size()); i++) {
      Node<M> *child = node->get_first_child() + i;
      if (node->is_final() && (child->get_value() != goal || !child->is_final())) {
        child->set_reason(Reason::PRUNING);
      }
    }
  }

  auto maybe_get_children(const Node<M>* node) const {
     if (node->has_children()) {
       return node->get_children();
     } else {
       return vector<pair<Position, Node<M>*>>{};
     }
  }

  void dump_node(ofstream& ofs, const Node<M>* node) const {
    auto children = maybe_get_children(node);
    ofs << static_cast<int>(node->get_value()) << " ";
    ofs << static_cast<int>(node->is_final()) << " ";
    ofs << static_cast<int>(node->get_proof()) << " ";
    ofs << static_cast<int>(node->get_disproof()) << " ";
    ofs << node->get_count() << " " << children.size() << " ";
    ofs << static_cast<int>(node->get_reason()) << " : ";
    for (const auto& [pos, p] : children) {
      ofs << pos << "  ";
    }
    ofs << "\n";
    for (const auto& [pos, p] : children) {
      dump_node(ofs, p);
    }
  }
};


#endif
