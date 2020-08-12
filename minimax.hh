#ifndef MINIMAX_HH
#define MINIMAX_HH

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

enum class Outcome {
  X_WINS,
  O_DRAWS,
  UNKNOWN
};

template<int N, int D>
constexpr Outcome known_outcome() {
  return Outcome::UNKNOWN;
}

template<>
constexpr Outcome known_outcome<3, 2>() {
  return Outcome::O_DRAWS;
}

template<>
constexpr Outcome known_outcome<4, 2>() {
  return Outcome::O_DRAWS;
}

template<>
constexpr Outcome known_outcome<3, 3>() {
  return Outcome::X_WINS;
}

template<>
constexpr Outcome known_outcome<4, 3>() {
  return Outcome::X_WINS;
}

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

template<typename T>
uint64_t shuffle_bits(T val) {
  uint64_t a = reinterpret_cast<uint64_t>(val);
  uint64_t ans = 0;
  for (int i = 0; i < 64; i++) {
    ans = (ans << 1) | (a & 1);
    a >>= 1;
  }
  return ans;
}

auto CompareBoardNode = [](const auto& a, const auto& b) {
  return shuffle_bits(a.node) < shuffle_bits(b.node);
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

template<int N, int D,
    typename Traversal = DFS<N, D, DefaultConfig::max_created>,
    typename Config = DefaultConfig,
    Outcome outcome = known_outcome<N, D>()>
class MiniMax {
 public:
  constexpr static Position board_size = BoardData<N, D>::board_size;
  constexpr static NodeCount M = Config::max_created;
  constexpr static Config config = Config();

  MiniMax(
      const State<N, D>& state,
      const BoardData<N, D>& data)
      :  state(state), data(data), solution(board_size), traversal(data, solution.get_root()) {
    if constexpr (config.should_log_evolution) {
      ofevolution.open("pnevolution.txt");
    }
  }
  const State<N, D>& state;
  const BoardData<N, D>& data;
  SolutionTree<M> solution;
  Traversal traversal;
  mutex next_m;
  mutex node_m;
  unordered_map<Zobrist, Node<M>*> zobrist;
  int nodes_visited = 0;
  int nodes_created = 1;
  int running_zobrist = 0;
  int running_final = 0;
  ofstream ofevolution;

  optional<BoardValue> play(State<N, D>& current_state, Turn turn) {
    auto ans = queue_play(BoardNode<N, D, M>{current_state, turn, solution.get_root()});
    config.debug << "Total nodes visited: "s << nodes_visited << "\n"s;
    config.debug << "Nodes in solution tree: "s << solution.real_count() << "\n"s;
    if constexpr (config.should_prune) {
      solution.prune();
    }
    config.debug << "Nodes in solution tree after pruning: "s << solution.real_count() << "\n"s;
    return ans;
  }

  const SolutionTree<M>& get_solution() const {
    return solution;
  }

  SolutionTree<M>& get_solution() {
    return solution;
  }

  optional<BoardValue> queue_play(BoardNode<N, D, M> root) {
    traversal.push_node(root);
    constexpr int slice = 1;
    vector<BoardNode<N, D, M>> nodes;
    nodes.reserve(slice);
    while (!traversal.empty() && nodes_visited < config.max_visited && nodes_created < config.max_created) {
      nodes.clear();
      for (int i = 0; i < slice && !traversal.empty(); i++) {
        nodes.emplace_back(traversal.pop_best());
      }
      for_each(begin(nodes), end(nodes), [&](auto node) {
        bool is_terminal = process_node(node);
        if (node.node->get_reason() == Reason::ZOBRIST) {
          running_zobrist++;
        }
        if (config.should_log_evolution) {
          ofevolution << solution.get_root()->get_proof() << " " <<  solution.get_root()->get_disproof() << " "
                      << node.node->get_depth() << " " << running_zobrist << " " << running_final << "\n";
        }
        traversal.retire(node, is_terminal);
      });
    }
    return root.node->get_value();
  }

  bool process_node(const BoardNode<N, D, M>& board_node) {
    auto& [current_state, turn, node] = board_node;
    report_progress(board_node);
    if (node->some_parent_final()) {
      node->set_reason(Reason::PRUNING);
      return false;
    }
    auto terminal_node = check_terminal_node(current_state, turn, node);
    if (terminal_node.has_value()) {
      return true;
    }
    traversal.push_parent(board_node, solution, nodes_created, config);
    return false;
  }

  optional<BoardValue> check_terminal_node(
      const State<N, D>& current_state, Turn turn, Node<M> *node) {
    Zobrist zob = current_state.get_zobrist();
    if (nodes_visited > config.max_visited) {
      return save_node(node, zob, BoardValue::UNKNOWN, Reason::OUT_OF_NODES, turn);
    }
    if (current_state.get_win_state()) {
      return save_node(node, zob, winner(turn), Reason::WIN, turn);
    }
    if (auto has_zobrist = zobrist.find(zob); has_zobrist != zobrist.end()) {
      return save_node(node, zob, has_zobrist->second->get_value(), Reason::ZOBRIST, turn);
    }
    auto open_positions = current_state.get_open_positions(to_mark(turn));
    if (open_positions.none()) {
      return save_node(node, zob, BoardValue::DRAW, Reason::DRAW, turn);
    }
    if (auto forced = check_chaining_strategy(current_state, turn); forced.has_value()) {
      return save_node(node, zob, *forced, Reason::CHAINING, turn);
    }
    if (auto forced = check_forced_win(current_state, turn, open_positions); forced.has_value()) {
      return save_node(node, zob, *forced, Reason::FORCED_WIN, turn);
    }
    return {};
  }

  BoardValue save_node(Node<M> *node, optional<Zobrist> node_zobrist,
      BoardValue value, Reason reason, Turn turn, bool is_final = true) {
    const lock_guard lock(node_m);
    return unsafe_save_node(node, node_zobrist, value, reason, turn, is_final);
  }

  BoardValue unsafe_save_node(Node<M> *node, optional<Zobrist> node_zobrist,
      BoardValue value, Reason reason, Turn turn, bool is_final = true) {
    node->set_reason(reason);
    node->set_value(value);
    if (!node->is_final() && is_final) {
      running_final++;
    }
    node->set_is_final(is_final);
    if (node_zobrist.has_value()) {
      if (reason == Reason::ZOBRIST) {
        node->set_zobrist_next(zobrist[*node_zobrist]->get_zobrist_next());
        node->set_zobrist_first(zobrist[*node_zobrist]);
        zobrist[*node_zobrist]->set_zobrist_next(node);
      } else {
        zobrist[*node_zobrist] = node;
      }
    }
    if (node->has_parent()) {
      for (auto sibling = node->get_zobrist_first(); sibling != nullptr; sibling = sibling->get_zobrist_next()) {
        update_parent_node(node, sibling->get_parent(), {}, value, reason, turn, is_final);
      }
    }
    return value;
  }

  void update_parent_node(
      Node<M> *node, Node<M> *parent, Zobrist node_zobrist,
      BoardValue value, Reason reason, Turn turn, bool is_final = true) {
    auto parent_turn = flip(turn);
    auto [new_parent_value, parent_is_final] = get_updated_parent_value(value, parent, parent_turn);
    bool old_is_final = parent->is_final();
    bool should_update = new_parent_value.has_value() || parent_is_final != old_is_final;
    if (should_update) {
      bool is_early = new_parent_value.has_value() && parent_is_final && !old_is_final;
      auto parent_reason = is_early ?
          Reason::MINIMAX_EARLY : Reason::MINIMAX_COMPLETE;
      auto updated_parent_value = new_parent_value.value_or(parent->get_value());
      unsafe_save_node(parent, {},
          updated_parent_value, parent_reason, flip(turn), parent_is_final);
    }
  }

  BoardValue winner(Mark mark) {
    return winner(to_turn(mark));
  }

  BoardValue winner(Turn turn) {
    return turn == Turn::X ? BoardValue::X_WIN : BoardValue::O_WIN;
  }

  bool is_final(BoardValue value, Turn turn) const {
    if (turn == Turn::X) {
      return value == BoardValue::X_WIN;
    } else {
      return value == BoardValue::DRAW || value == BoardValue::O_WIN;
    }
  }

  template<typename T>
  bool is_final_candidate(T& children, optional<BoardValue> new_value) {
    return find_if(begin(children), end(children), [&](const auto& child) {
      return child.second->get_value() == new_value && child.second->is_final();
    }) != end(children);
  }

  pair<optional<BoardValue>, bool> get_updated_parent_value(
      optional<BoardValue> child_value,
      const Node<M> *parent,
      Turn parent_turn) {
    assert(child_value != BoardValue::UNKNOWN);
    auto children = parent->get_children();
    auto new_value = parent_turn == Turn::X ? parent->best_child_X() : parent->best_child_O();
    bool final_candidate = is_final_candidate(children, new_value);
    bool all_children_final = all_of(begin(children), end(children), [&](const auto& child) {
      return child.second->is_final();
    });
    bool parent_is_final = all_children_final ||
        (final_candidate && is_final(*new_value, parent_turn));
    if (new_value != parent->get_value()) {
      return {new_value, parent_is_final};
    } else {
      return {{}, parent_is_final};
    }
  }

  void report_progress(const BoardNode<N, D, M>& board_node) {
    if ((nodes_visited % 1000) == 0) {
      config.debug << "visited "s << nodes_visited << "\t"s;
      config.debug << "created "s << nodes_created << "\t"s;
      //double value = traversal.estimate_work(board_node.node);
      double value = solution.get_root()->work;
      ostringstream oss;
      oss << setprecision(2) << value * 100.0;
      config.debug << "done : "s << oss.str() << "%\n"s;
    }
    nodes_visited++;
  }

  optional<BoardValue> check_chaining_strategy(const State<N, D>& current_state, Turn turn) {
    auto c = ChainingStrategy(current_state);
    auto pos = c.search(to_mark(turn));
    static int max_visited = 0;
    if (c.visited > max_visited) {
      max_visited = c.visited;
      config.debug << "new record "s << max_visited << "\n"s;
    }
    if (pos.has_value()) {
      return winner(turn);
    }
    return {};
  }

  template<typename B>
  optional<BoardValue> check_forced_win(const State<N, D>& current_state, Turn turn, const B& open_positions) {
    auto s = ForcingMove<N, D>(current_state);
    auto forcing = s.check(to_mark(turn), open_positions);
    if (forcing.first.has_value()) {
      if (forcing.second == to_mark(turn)) {
        return winner(turn);
      }
    }
    return {};
  }

};

#endif
