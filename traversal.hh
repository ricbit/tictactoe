#ifndef TRAVERSAL_HH
#define TRAVERSAL_HH

#include <iostream>
#include <tuple>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <set>
#include <map>
#include <unordered_map>
#include <queue>
#include <cassert>
#include <bitset>
#include <execution>
#include <list>
#include <stack>
#include <mutex>
#include <numeric>
#include "semantic.hh"
#include "boarddata.hh"
#include "state.hh"
#include "solutiontree.hh"
#include "strategies.hh"

template<int N, int D, int M>
class DFS {
 public:
  explicit DFS(const BoardData<N, D>& data, Node<M> *root) : data(data), root(root) {
  }
  void push_node(BoardNode<N, D, M> node) {
    next.push(node);
  }
  template<typename S, typename Config>
  void push_parent(BoardNode<N, D, M> board_node, S& solution, int &nodes_created, Config& config) {
    ChildrenBuilder<N, D, Config> builder;
    auto children = builder.build_children(board_node, solution, nodes_created);
    for (auto& child : children) {
      push_node(child);
    }
  }
  BoardNode<N, D, M> pop_best() {
    BoardNode<N, D, M> node = next.top();
    next.pop();
    return node;
  }
  bool empty() const {
    return next.empty();
  }
  void retire(const BoardNode<N, D, M>& node, bool is_terminal) {
    // empty
  }
  float estimate_work(const Node<M> *node) {
    return node->estimate_work();
  }
 private:
  stack<BoardNode<N, D, M>> next;
  const BoardData<N, D>& data;
  Node<M> *root;
};

template<int N, int D, int M>
class BFS {
 public:
  explicit BFS(const BoardData<N, D>& data, Node<M> *root) : data(data), root(root) {
  }
  void push_node(BoardNode<N, D, M> node) {
    next.push(node.node);
  }
  template<typename S, typename Config>
  void push_parent(BoardNode<N, D, M> board_node, S& solution, int &nodes_created, Config& config) {
    ChildrenBuilder<N, D, Config> builder;
    auto children = builder.build_children(board_node, solution, nodes_created);
    for (auto& child : children) {
      push_node(child);
    }
  }
  BoardNode<N, D, M> pop_best() {
    auto node = next.front();
    next.pop();
    return BoardNode<N, D, M>{node->rebuild_state(data), node->get_turn(), node};
  }
  bool empty() const {
    return next.empty();
  }
  void retire(const BoardNode<N, D, M>& node, bool is_terminal) {
    // empty
  }
  float estimate_work(const Node<M> *node) {
    return node->estimate_work();
  }
 private:
  queue<Node<M>*> next;
  const BoardData<N, D>& data;
  Node<M> *root;
};

template<int N, int D, int M>
class PNSearch {
  optional<Node<M>*> descent;
 public:
  explicit PNSearch(const BoardData<N, D>& data, Node<M> *root) : data(data), root(root) {
  }
  void push_node(BoardNode<N, D, M> board_node) {
  }
  template<typename S, typename Config>
  void push_parent(BoardNode<N, D, M> board_node, S& solution, int &nodes_created, Config& config) {
    ChildrenBuilder<N, D, Config> builder;
    auto children = builder.build_children(board_node, solution, nodes_created);
    for (auto& child : children) {
      push_node(child);
    }
  }
  BoardNode<N, D, M> pop_best() {
    auto board_node = choose_best_pn_node();
    return board_node;
  }
  bool empty() const {
    bool is_final = root->is_final();
    if (is_final) {
      assert(root->get_proof() == 0_pn || root->get_disproof() == 0_pn);
    }
    return is_final;
  }
  void retire(const BoardNode<N, D, M>& board_node, bool is_terminal) {
    auto& node = board_node.node;
    if (is_terminal) {
      if (node->get_value() == BoardValue::X_WIN) {
        node->set_proof(0_pn);
        node->set_disproof(INFTY);
      } else if (node->get_value() == BoardValue::O_WIN || node->get_value() == BoardValue::DRAW) {
        node->set_disproof(0_pn);
        node->set_proof(INFTY);
      } else {
        assert(false);
      }
    }
    update_pn_value(node, board_node.turn);
  }
  float estimate_work(const Node<M> *node) {
    return node->estimate_work();
  }
 private:
  BoardNode<N, D, M> choose_best_pn_node() {
    return search_or_node(root);
    /*if (!descent.has_value()) {
      auto chosen_node = search_or_node(root);
      descent = chosen_node.node;
      return chosen_node;
    } else {
      if ((*descent)->get_children().empty()) {
        descent.reset();
        return pop_best();
      }
      auto children = (*descent)->get_children();
      int size = children.size();
      descent = children[rand() % size].second;
      return choose(BoardNode<N, D, M>{(*descent)->rebuild_state(data), (*descent)->get_turn(), *descent});
    }*/
  }
  void update_pn_value(Node<M> *node, Turn turn) {
    auto children = node->get_children();
    if (node->is_final()) {
      node->work = 1.0f;
    } else {
      float size = children.size();
      node->work = accumulate(begin(children), end(children), 0.0f, [](const auto& a, const auto& b) {
        return a + b.second->work;
      }) / size;
    }
    if (!children.empty()) {
      if (turn == Turn::O) {
        auto proof = accumulate(begin(children), end(children), 0_pn, [](const auto& a, const auto& b) {
          return ProofNumber{a + b.second->get_proof()};
        });
        node->set_proof(clamp(proof, 0_pn, INFTY));
        node->set_disproof(min_disproof(children)->get_disproof());
      } else {
        auto disproof = accumulate(begin(children), end(children), 0_pn, [](const auto& a, const auto& b) {
          return ProofNumber{a + b.second->get_disproof()};
        });
        node->set_disproof(clamp(disproof, 0_pn, INFTY));
        node->set_proof(min_proof(children)->get_proof());
      }
    }
    if (node->has_parent()) {
      for (auto sibling = node->get_zobrist_first(); sibling != nullptr; sibling = sibling->get_zobrist_next()) {
        update_pn_value(sibling->get_parent(), flip(turn));
      }
    }
  }
  BoardNode<N, D, M> search_or_node(Node<M> *node) {
    auto children = node->get_children();
    if (children.empty()) {
      return BoardNode<N, D, M>{node->rebuild_state(data), node->get_turn(), node};
    }
    return search_and_node(min_proof(children));
  }
  BoardNode<N, D, M> search_and_node(Node<M> *node) {
    auto children = node->get_children();
    if (children.empty()) {
      return BoardNode<N, D, M>{node->rebuild_state(data), node->get_turn(), node};
    }
    return search_or_node(min_disproof(children));
  }
  template<typename C>
  auto min_proof(const C& children) {
    return min_element(begin(children), end(children), [](const auto &a, const auto& b) {
      return a.second->get_proof() < b.second->get_proof();
    })->second;
  }
  template<typename C>
  auto min_disproof(const C& children) {
    return min_element(begin(children), end(children), [](const auto &a, const auto& b) {
      return a.second->get_disproof() < b.second->get_disproof();
    })->second;
  }
  const BoardData<N, D>& data;
  Node<M> *root;
};

#endif
