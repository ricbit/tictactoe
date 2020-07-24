#ifndef MINIMAX_HH
#define MINIMAX_HH

#include <iostream>
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

class IdentityHash {
 public:
  size_t operator()(const Zobrist& zobrist) const noexcept {
    return zobrist;
  }
};

class DummyCout {
 public:
  template<typename T>
  const DummyCout& operator<<(const T& x) const {
    return *this;
  }
  template<typename T>
  const DummyCout& operator<<(T& x) const {
    return *this;
  }
};

struct DefaultConfig {
  int max_nodes = 1000000;
  DummyCout debug;
};

template<int N, int D>
struct BoardNode {
  State<N, D> current_state;
  Turn turn;
  SolutionTree::Node *node;
};

template<int N, int D>
class DFS {
 public:
  explicit DFS(SolutionTree::Node *root) : root(root) {
  }
  void push(BoardNode<N, D> node) {
    next.push(node);
  }
  BoardNode<N, D> pop_best() {
    BoardNode<N, D> node = next.top();
    next.pop();
    return node;
  }
  bool empty() const {
    return next.empty();
  }
  void retire(const BoardNode<N, D>& node, bool is_terminal) {
    // empty
  }
 private:
  stack<BoardNode<N, D>> next;
  SolutionTree::Node *root;
};

template<int N, int D>
class BFS {
 public:
  explicit BFS(SolutionTree::Node *root) : root(root) {
  }
  void push(BoardNode<N, D> node) {
    next.push(node);
  }
  BoardNode<N, D> pop_best() {
    BoardNode<N, D> node = next.front();
    next.pop();
    return node;
  }
  bool empty() const {
    return next.empty();
  }
  void retire(const BoardNode<N, D>& node, bool is_terminal) {
    // empty
  }
 private:
  queue<BoardNode<N, D>> next;
  SolutionTree::Node *root;
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

template<int N, int D>
class RandomTraversal {
 public:
  explicit RandomTraversal(SolutionTree::Node *root) : root(root) {
  }
  void push(BoardNode<N, D> node) {
    next.insert(node);
  }
  BoardNode<N, D> pop_best() {
    BoardNode<N, D> node = move(next.extract(next.begin()).value());
    return node;
  }
  bool empty() const {
    return next.empty();
  }
  void retire(const BoardNode<N, D>& node, bool is_terminal) {
    // empty
  }
 private:
  set<BoardNode<N, D>, decltype(CompareBoardNode)> next;
  SolutionTree::Node *root;
};

template<int N, int D>
class PNSearch {
 public:
  explicit PNSearch(const BoardData<N, D>& data, SolutionTree::Node *root) : data(data), root(root) {
  }
  void push(BoardNode<N, D> board_node) {
  }
  BoardNode<N, D> pop_best() {
    cout << "\n------\n";
    return search_or_node(root);
  }
  bool empty() const {
    return root->is_final();
  }
  void retire(const BoardNode<N, D>& board_node, bool is_terminal) {
    auto& node = board_node.node;
    if (is_terminal) {
      if (node->value == BoardValue::X_WIN) {
        node->proof = 0_pn;
        node->disproof = INFTY;
      } else if (node->value == BoardValue::O_WIN || node->value == BoardValue::DRAW) {
        node->disproof = 0_pn;
        node->proof = INFTY;
      } else {
        assert(false);
      }
    }
    update_pn_value(node, board_node.turn);
    cout << "after ";
    cout << "proof : " << board_node.node->proof << " disproof: " << board_node.node->disproof << "\n";
    cout << "children : " << board_node.node->children.size() << "\n";
  }
  void update_pn_value(SolutionTree::Node *node, Turn turn) {
    if (node == nullptr) {
      return;
    }
    if (!node->children.empty()) {
      if (turn == Turn::O) {
        auto proof = accumulate(begin(node->children), end(node->children), 0_pn, [](const auto& a, const auto& b) {
          return ProofNumber{a + b.second->proof};
        });
        node->proof = clamp(proof, 0_pn, INFTY);
        node->disproof = min_element(begin(node->children), end(node->children), [](const auto &a, const auto& b) {
          return a.second->disproof < b.second->disproof;
        })->second->disproof;
      } else {
        auto disproof = accumulate(begin(node->children), end(node->children), 0_pn, [](const auto& a, const auto& b) {
          return ProofNumber{a + b.second->disproof};
        });
        node->disproof = clamp(disproof, 0_pn, INFTY);
        node->proof = min_element(begin(node->children), end(node->children), [](const auto &a, const auto& b) {
          return a.second->proof < b.second->proof;
        })->second->proof;
      }
    }
    update_pn_value(node->parent, flip(turn));
  }
 private:
  BoardNode<N, D> choose(BoardNode<N, D> board_node) {
    board_node.current_state.print();
    cout << "before ";
    cout << "proof : " << board_node.node->proof << " disproof: " << board_node.node->disproof << "\n";
    cout << "children : " << board_node.node->children.size() << "\n";
    cout << "turn : " << board_node.turn << "\n";
    return board_node;
  }
  BoardNode<N, D> search_or_node(SolutionTree::Node *node) {
    if (node->children.empty()) {
      return choose(BoardNode<N, D>{node->rebuild_state(data), node->get_turn(), node});
    }
    return search_and_node(min_element(begin(node->children), end(node->children), [](const auto &a, const auto& b) {
      return a.second->proof < b.second->proof;
    })->second.get());
  }
  BoardNode<N, D> search_and_node(SolutionTree::Node *node) {
    if (node->children.empty()) {
      return choose(BoardNode<N, D>{node->rebuild_state(data), node->get_turn(), node});
    }
    return search_or_node(min_element(begin(node->children), end(node->children), [](const auto &a, const auto& b) {
      return a.second->disproof < b.second->disproof;
    })->second.get());
  }
  const BoardData<N, D>& data;
  SolutionTree::Node *root;
};

template<int N, int D,
    typename Traversal = DFS<N, D>,
    typename Config = DefaultConfig,
    Outcome outcome = known_outcome<N, D>()>
class MiniMax {
 public:
  constexpr static Position board_size = BoardData<N, D>::board_size;

  explicit MiniMax(
    const State<N, D>& state,
    const BoardData<N, D>& data)
    :  state(state), data(data), nodes_visited(0), solution(board_size), traversal(data, solution.get_root()) {
  }
  const State<N, D>& state;
  const BoardData<N, D>& data;
  int nodes_visited;
  SolutionTree solution;
  Traversal traversal;
  mutex next_m;
  mutex node_m;
  unordered_map<Zobrist, BoardValue, IdentityHash> zobrist;
  constexpr static Config config = Config();

  optional<BoardValue> play(State<N, D>& current_state, Turn turn) {
    auto ans = queue_play(BoardNode{current_state, turn, solution.get_root()});
    config.debug << "Total nodes visited: "s << nodes_visited << "\n"s;
    config.debug << "Nodes in solution tree: "s << solution.real_count() << "\n"s;
    //solution.prune();
    config.debug << "Nodes in solution tree after pruning: "s << solution.real_count() << "\n"s;
    return ans;
  }

  const SolutionTree& get_solution() const {
    return solution;
  }

  SolutionTree& get_solution() {
    return solution;
  }

  optional<BoardValue> queue_play(BoardNode<N, D> root) {
    traversal.push(root);
    constexpr int slice = 1;
    vector<BoardNode<N, D>> nodes;
    nodes.reserve(slice);
    while (!traversal.empty() && nodes_visited < config.max_nodes) {
      nodes.clear();
      for (int i = 0; i < slice && !traversal.empty(); i++) {
        nodes.emplace_back(traversal.pop_best());
      }
      for_each(begin(nodes), end(nodes), [&](auto node) {
        bool is_terminal = process_node(node);
        traversal.retire(node, is_terminal);
      });
    }
    return root.node->value;
  }

  bool process_node(const BoardNode<N, D>& board_node) {
    auto& [current_state, turn, node] = board_node;
    report_progress(board_node);
    if (node->some_parent_final()) {
      node->reason = SolutionTree::Reason::PRUNING;
      return false;
    }
    auto terminal_node = check_terminal_node(current_state, turn, node);
    if (terminal_node.has_value()) {
      return true;
    }

    auto open_positions = current_state.get_open_positions(to_mark(turn));
    auto [sorted, child_state] = get_children(current_state, turn, open_positions);
    int size = sorted.size();
    for (int i = size - 1; i >= 0; i--) {
      const auto& child = child_state[i];
      node->children.emplace_back(sorted[i].second, make_unique<SolutionTree::Node>(node, open_positions.count()));
      auto child_node = node->children.rbegin()->second.get();
      lock_guard lock(next_m);
      traversal.push(BoardNode{child, flip(turn), child_node});
    }
    return false;
  }

  optional<BoardValue> check_terminal_node(const State<N, D>& current_state, Turn turn, SolutionTree::Node *node) {
    Zobrist zob = current_state.get_zobrist();
    if (nodes_visited > config.max_nodes) {
      return save_node(node, zob, BoardValue::UNKNOWN, SolutionTree::Reason::OUT_OF_NODES, turn);
    }
    if (current_state.get_win_state()) {
      return save_node(node, zob, winner(turn), SolutionTree::Reason::WIN, turn);
    }
    if (auto has_zobrist = zobrist.find(zob); has_zobrist != zobrist.end()) {
      return save_node(node, zob, has_zobrist->second, SolutionTree::Reason::ZOBRIST, turn);
    }
    auto open_positions = current_state.get_open_positions(to_mark(turn));
    if (open_positions.none()) {
      return save_node(node, zob, BoardValue::DRAW, SolutionTree::Reason::DRAW, turn);
    }
    if (auto forced = check_chaining_strategy(current_state, turn); forced.has_value()) {
      return save_node(node, zob, *forced, SolutionTree::Reason::CHAINING, turn);
    }
    if (auto forced = check_forced_win(current_state, turn, open_positions); forced.has_value()) {
      return save_node(node, zob, *forced, SolutionTree::Reason::FORCED_WIN, turn);
    }
    return {};
  }

  template<typename B>
  pair<vector<pair<int, Position>>, bag<State<N, D>>> get_children(
      const State<N, D>& current_state, Turn turn, B open_positions) {
    vector<pair<int, Position>> sorted;
    bag<State<N, D>> child_state;

    auto s = ForcingMove<N, D>(current_state);
    auto forcing = s.check(to_mark(turn), open_positions);
    if (forcing.first.has_value()) {
      assert(forcing.second != to_mark(turn));
      child_state.emplace_back(current_state);
      State<N, D>& cloned = *child_state.begin();
      bool game_ended = cloned.play(*forcing.first, to_mark(turn));
      assert(!game_ended);
      int dummy_score = 1;
      sorted = {{dummy_score, *forcing.first}};
    } else {
      child_state.reserve(open_positions.count());
      sorted = get_sorted_positions(current_state, open_positions);
      for (const auto& [score, pos] : sorted) {
        child_state.emplace_back(current_state);
        child_state.rbegin()->play(pos, to_mark(turn));
      }
    }
    return {sorted, child_state};
  }

  BoardValue save_node(SolutionTree::Node *node, Zobrist node_zobrist,
      BoardValue value, SolutionTree::Reason reason, Turn turn, bool is_final = true) {
    const lock_guard lock(node_m);
    return unsafe_save_node(node, node_zobrist, value, reason, turn, is_final);
  }

  BoardValue unsafe_save_node(SolutionTree::Node *node, Zobrist node_zobrist,
      BoardValue value, SolutionTree::Reason reason, Turn turn, bool is_final = true) {
    node->reason = reason;
    node->value = value;
    node->node_final = is_final;
    if (node->is_final()) {
      zobrist[node_zobrist] = value;
    }
    if (node->parent != nullptr) {
      auto parent_turn = flip(turn);
      auto [new_parent_value, parent_is_final] = get_updated_parent_value(value, node->parent, parent_turn);
      bool old_is_final = node->parent->is_final();
      bool should_update = new_parent_value.has_value() || parent_is_final != old_is_final;
      if (should_update) {
        bool is_early = new_parent_value.has_value() && parent_is_final && !old_is_final;
        auto parent_reason = is_early ?
            SolutionTree::Reason::MINIMAX_EARLY : SolutionTree::Reason::MINIMAX_COMPLETE;
        auto updated_parent_value = new_parent_value.value_or(node->parent->value);
        unsafe_save_node(
            node->parent, node->parent->zobrist, updated_parent_value, parent_reason, flip(turn), parent_is_final);
      }
    }
    return value;
  }

  BoardValue winner(Mark mark) {
    return mark == Mark::X ? BoardValue::X_WIN : BoardValue::O_WIN;
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

  pair<optional<BoardValue>, bool> get_updated_parent_value(
      optional<BoardValue> child_value,
      const SolutionTree::Node *parent,
      Turn parent_turn) {
    assert(child_value != BoardValue::UNKNOWN);
    auto new_value = parent_turn == Turn::X ? parent->min_child() : parent->max_child();
    bool final_candidate = find_if(begin(parent->children), end(parent->children), [&](const auto& child) {
      return child.second->value == new_value && child.second->is_final();
    }) != end(parent->children);
    bool all_children_final = all_of(begin(parent->children), end(parent->children), [&](const auto& child) {
      return child.second->is_final();
    });
    bool parent_is_final = all_children_final ||
        (final_candidate && is_final(*new_value, parent_turn));
    if (new_value != parent->value) {
      return {new_value, parent_is_final};
    } else {
      return {{}, parent_is_final};
    }
  }

  template<typename B>
  vector<pair<int, Position>> get_sorted_positions(const State<N, D>& current_state, const B& open) {
    vector<pair<int, Position>> paired;
    paired.reserve(open.count());
    uniform_positions(paired, open, current_state);
    return paired;
  }

  template<typename B>
  void uniform_positions(vector<pair<int, Position>>& paired,
      const B& open, const State<N, D>& current_state) {
    for (Position pos : open.all()) {
      paired.push_back(make_pair(
          current_state.get_current_accumulation(pos), pos));
    }
    sort(rbegin(paired), rend(paired));
  }

  void report_progress(const BoardNode<N, D>& board_node) {
    if ((nodes_visited % 1000) == 0) {
      config.debug << "id "s << nodes_visited << "\t"s;
      double value = board_node.node->estimate_work();
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
