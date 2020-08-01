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
  constexpr static NodeCount max_visited = 1'000'000_nc;
  constexpr static NodeCount max_created = 1'000'000_nc;
  DummyCout debug;
  bool should_prune = true;
};

template<int N, int D, int M>
struct BoardNode {
  State<N, D> current_state;
  Turn turn;
  SolutionTree<M>::Node *node;
};

template<int N, int D, int M>
class DFS {
 public:
  explicit DFS(const BoardData<N, D>& data, SolutionTree<M>::Node *root) : data(data), root(root) {
  }
  void push(BoardNode<N, D, M> node) {
    next.push(node);
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
  float estimate_work(const SolutionTree<M>::Node *node) {
    return node->estimate_work();
  }
 private:
  stack<BoardNode<N, D, M>> next;
  const BoardData<N, D>& data;
  SolutionTree<M>::Node *root;
};

template<int N, int D, int M>
class BFS {
 public:
  explicit BFS(const BoardData<N, D>& data, SolutionTree<M>::Node *root) : data(data), root(root) {
  }
  void push(BoardNode<N, D, M> node) {
    next.push(node.node);
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
  float estimate_work(const SolutionTree<M>::Node *node) {
    return node->estimate_work();
  }
 private:
  queue<typename SolutionTree<M>::Node*> next;
  const BoardData<N, D>& data;
  SolutionTree<M>::Node *root;
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
class RandomTraversal {
 public:
  explicit RandomTraversal(const BoardData<N, D>& data, SolutionTree<M>::Node *root) : data(data), root(root) {
  }
  void push(BoardNode<N, D, M> node) {
    next.insert(node);
  }
  BoardNode<N, D, M> pop_best() {
    BoardNode<N, D, M> node = move(next.extract(next.begin()).value());
    return node;
  }
  bool empty() const {
    return next.empty();
  }
  void retire(const BoardNode<N, D, M>& node, bool is_terminal) {
    // empty
  }
  float estimate_work(const SolutionTree<M>::Node *node) {
    return node->estimate_work();
  }
 private:
  set<BoardNode<N, D, M>, decltype(CompareBoardNode)> next;
  const BoardData<N, D>& data;
  SolutionTree<M>::Node *root;
};

template<int N, int D, int M>
class PNSearch {
  optional<typename SolutionTree<M>::Node*> descent;
  bag<tuple<ProofNumber, ProofNumber, int>> pn_evolution;
 public:
  explicit PNSearch(const BoardData<N, D>& data, SolutionTree<M>::Node *root) : data(data), root(root) {
  }
  ~PNSearch() {
    save_evolution();
  }
  void push(BoardNode<N, D, M> board_node) {
  }
  BoardNode<N, D, M> pop_best() {
    auto board_node = choose_best_pn_node();
    pn_evolution.push_back({root->proof, root->disproof, board_node.node->get_depth()});
    return board_node;
  }
  bool empty() const {
    bool is_final = root->is_final();
    if (is_final) {
      assert(root->proof == 0 || root->disproof == 0);
    }
    return is_final;
  }
  void retire(const BoardNode<N, D, M>& board_node, bool is_terminal) {
    auto& node = board_node.node;
    if (is_terminal) {
      if (node->get_value() == BoardValue::X_WIN) {
        node->proof = 0_pn;
        node->disproof = INFTY;
      } else if (node->get_value() == BoardValue::O_WIN || node->get_value() == BoardValue::DRAW) {
        node->disproof = 0_pn;
        node->proof = INFTY;
      } else {
        assert(false);
      }
    }
    update_pn_value(node, board_node.turn);
  }
  float estimate_work(const SolutionTree<M>::Node *node) {
    return node->estimate_work();
  }
 private:
  BoardNode<N, D, M> choose_best_pn_node() {
    //return search_or_node(root);
    if (!descent.has_value()) {
      auto chosen_node = search_or_node(root);
      descent = chosen_node.node;
      return chosen_node;
    } else {
      if ((*descent)->children.empty()) {
        descent.reset();
        return pop_best();
      }
      int size = (*descent)->children.size();
      descent = (*descent)->children[rand() % size].second;
      return choose(BoardNode<N, D, M>{(*descent)->rebuild_state(data), (*descent)->get_turn(), *descent});
    }
  }
  void save_evolution() const {
    ofstream ofs("pnevolution.txt");
    for (const auto& pn : pn_evolution) {
      ofs << get<0>(pn) << " " << get<1>(pn) << " " << get<2>(pn) << "\n";
    }
  }
  void update_pn_value(SolutionTree<M>::Node *node, Turn turn) {
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
    if (node->has_parent()) {
      update_pn_value(node->get_parent(), flip(turn));
    }
  }
  BoardNode<N, D, M> choose(BoardNode<N, D, M> board_node) {
    return board_node;
  }
  BoardNode<N, D, M> search_or_node(SolutionTree<M>::Node *node) {
    if (node->children.empty()) {
      return choose(BoardNode<N, D, M>{node->rebuild_state(data), node->get_turn(), node});
    }
    return search_and_node(min_element(begin(node->children), end(node->children), [](const auto &a, const auto& b) {
      return a.second->proof < b.second->proof;
    })->second);
  }
  BoardNode<N, D, M> search_and_node(SolutionTree<M>::Node *node) {
    if (node->children.empty()) {
      return choose(BoardNode<N, D, M>{node->rebuild_state(data), node->get_turn(), node});
    }
    return search_or_node(min_element(begin(node->children), end(node->children), [](const auto &a, const auto& b) {
      return a.second->disproof < b.second->disproof;
    })->second);
  }
  const BoardData<N, D>& data;
  SolutionTree<M>::Node *root;
};

template<int N, int D,
    typename Traversal = DFS<N, D, DefaultConfig::max_created>,
    typename Config = DefaultConfig,
    Outcome outcome = known_outcome<N, D>()>
class MiniMax {
 public:
  constexpr static Position board_size = BoardData<N, D>::board_size;

  MiniMax(
    const State<N, D>& state,
    const BoardData<N, D>& data)
    :  state(state), data(data), solution(board_size), traversal(data, solution.get_root()) {
  }
  const State<N, D>& state;
  const BoardData<N, D>& data;
  constexpr static NodeCount M = Config::max_created;
  SolutionTree<M> solution;
  Traversal traversal;
  mutex next_m;
  mutex node_m;
  unordered_map<Zobrist, BoardValue, IdentityHash> zobrist;
  int nodes_visited = 0;
  int nodes_created = 1;
  constexpr static Config config = Config();

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
    traversal.push(root);
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
        traversal.retire(node, is_terminal);
      });
    }
    return root.node->get_value();
  }

  bool process_node(const BoardNode<N, D, M>& board_node) {
    auto& [current_state, turn, node] = board_node;
    report_progress(board_node);
    if (node->some_parent_final()) {
      node->set_reason(SolutionTree<M>::Reason::PRUNING);
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
      if (nodes_created == config.max_created) {
        return false;
      }
      nodes_created++;
      const auto& child = child_state[i];
      node->children.emplace_back(sorted[i].second, solution.create_node(node, open_positions.count()));
      auto child_node = node->children.rbegin()->second;
      lock_guard lock(next_m);
      traversal.push(BoardNode<N, D, M>{child, flip(turn), child_node});
    }
    return false;
  }

  optional<BoardValue> check_terminal_node(const State<N, D>& current_state, Turn turn, SolutionTree<M>::Node *node) {
    Zobrist zob = current_state.get_zobrist();
    if (nodes_visited > config.max_visited) {
      return save_node(node, zob, BoardValue::UNKNOWN, SolutionTree<M>::Reason::OUT_OF_NODES, turn);
    }
    if (current_state.get_win_state()) {
      return save_node(node, zob, winner(turn), SolutionTree<M>::Reason::WIN, turn);
    }
    if (auto has_zobrist = zobrist.find(zob); has_zobrist != zobrist.end()) {
      return save_node(node, zob, has_zobrist->second, SolutionTree<M>::Reason::ZOBRIST, turn);
    }
    auto open_positions = current_state.get_open_positions(to_mark(turn));
    if (open_positions.none()) {
      return save_node(node, zob, BoardValue::DRAW, SolutionTree<M>::Reason::DRAW, turn);
    }
    if (auto forced = check_chaining_strategy(current_state, turn); forced.has_value()) {
      return save_node(node, zob, *forced, SolutionTree<M>::Reason::CHAINING, turn);
    }
    if (auto forced = check_forced_win(current_state, turn, open_positions); forced.has_value()) {
      return save_node(node, zob, *forced, SolutionTree<M>::Reason::FORCED_WIN, turn);
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

  BoardValue save_node(SolutionTree<M>::Node *node, Zobrist node_zobrist,
      BoardValue value, SolutionTree<M>::Reason reason, Turn turn, bool is_final = true) {
    const lock_guard lock(node_m);
    return unsafe_save_node(node, node_zobrist, value, reason, turn, is_final);
  }

  BoardValue unsafe_save_node(SolutionTree<M>::Node *node, Zobrist node_zobrist,
      BoardValue value, SolutionTree<M>::Reason reason, Turn turn, bool is_final = true) {
    node->set_reason(reason);
    node->set_value(value);
    node->set_is_final(is_final);
    if (node->is_final()) {
      zobrist[node_zobrist] = value;
    }
    /*if (reason != SolutionTree<M>::Reason::ZOBRIST) {
      zobrist[node_zobrist] = value;
    } else {
      node->zobrist = zobrist[node_zobrist];
    }*/
    if (node->has_parent()) {
      auto parent_turn = flip(turn);
      auto [new_parent_value, parent_is_final] = get_updated_parent_value(value, node->get_parent(), parent_turn);
      bool old_is_final = node->get_parent()->is_final();
      bool should_update = new_parent_value.has_value() || parent_is_final != old_is_final;
      if (should_update) {
        bool is_early = new_parent_value.has_value() && parent_is_final && !old_is_final;
        auto parent_reason = is_early ?
            SolutionTree<M>::Reason::MINIMAX_EARLY : SolutionTree<M>::Reason::MINIMAX_COMPLETE;
        auto updated_parent_value = new_parent_value.value_or(node->get_parent()->get_value());
        unsafe_save_node(node->get_parent(), node->get_parent()->get_zobrist(),
            updated_parent_value, parent_reason, flip(turn), parent_is_final);
      }
    }
    return value;
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
      const SolutionTree<M>::Node *parent,
      Turn parent_turn) {
    assert(child_value != BoardValue::UNKNOWN);
    auto new_value = parent_turn == Turn::X ? parent->best_child_X() : parent->best_child_O();
    bool final_candidate = is_final_candidate(parent->children, new_value);
    bool all_children_final = all_of(begin(parent->children), end(parent->children), [&](const auto& child) {
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

  void report_progress(const BoardNode<N, D, M>& board_node) {
    if ((nodes_visited % 1000) == 0) {
      config.debug << "visited "s << nodes_visited << "\t"s;
      config.debug << "created "s << nodes_created << "\t"s;
      double value = traversal.estimate_work(board_node.node);
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
