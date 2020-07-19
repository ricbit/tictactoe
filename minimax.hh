#ifndef MINIMAX_HH
#define MINIMAX_HH

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <set>
#include <queue>
#include <cassert>
#include <bitset>
#include <execution>
#include <list>
#include <stack>
#include <mutex>
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

template<int N, int D, typename Config = DefaultConfig, Outcome outcome = known_outcome<N, D>()>
class MiniMax {
 public:
  constexpr static Position board_size = BoardData<N, D>::board_size;

  struct BoardNode {
    State<N, D> current_state;
    Mark mark;
    SolutionTree::Node *node;
  };

  explicit MiniMax(
    const State<N, D>& state,
    const BoardData<N, D>& data)
    :  state(state), data(data),
       nodes_visited(0),
       solution(board_size) {
  }
  const State<N, D>& state;
  const BoardData<N, D>& data;
  int nodes_visited;
  SolutionTree solution;
  stack<BoardNode> next;
  mutex next_m;
  mutex node_m;
  unordered_map<Zobrist, BoardValue, IdentityHash> zobrist;
  constexpr static Config config = Config();

  optional<BoardValue> play(State<N, D>& current_state, Mark mark) {
    auto ans = queue_play(BoardNode{current_state, mark, solution.get_root()});
    config.debug << "Total nodes visited: "s << nodes_visited << "\n"s;
    config.debug << "Nodes in solution tree: "s << solution.real_count() << "\n"s;
    solution.prune();
    config.debug << "Nodes in solution tree after pruning: "s << solution.real_count() << "\n"s;
    return ans;
  }

  const SolutionTree& get_solution() const {
    return solution;
  }

  SolutionTree& get_solution() {
    return solution;
  }

  optional<BoardValue> queue_play(BoardNode root) {
    assert(next.empty());
    next.push(root);
    constexpr int slice = 1;
    vector<BoardNode> nodes;
    nodes.reserve(slice);
    while (!next.empty()) {
      nodes.clear();
      for (int i = 0; i < slice && !next.empty(); i++) {
        nodes.emplace_back(next.top());
        next.pop();
      }
      for_each(begin(nodes), end(nodes), [&](auto node) {
        process_node(node);
      });
    }
    return root.node->value;
  }

  void process_node(BoardNode board_node) {
    auto& [current_state, mark, node] = board_node;
    report_progress();
    if (node->is_parent_final()) {
      node->reason = SolutionTree::Reason::PRUNING;
      return;
    }
    auto terminal_node = check_terminal_node(current_state, mark, node);
    if (terminal_node.has_value()) {
      return;
    }

    auto open_positions = current_state.get_open_positions(mark);
    auto [sorted, child_state] = get_children(current_state, mark, open_positions);
    int size = sorted.size();
    for (int i = size - 1; i >= 0; i--) {
      const auto& child = child_state[i];
      node->children.emplace_back(sorted[i].second, make_unique<SolutionTree::Node>(node, open_positions.count()));
      auto child_node = node->children.rbegin()->second.get();
      auto child_board_node = BoardNode{child, flip(mark), child_node};
      lock_guard lock(next_m);
      next.push(child_board_node);
    }
  }

  optional<BoardValue> check_terminal_node(State<N, D>& current_state, Mark mark, SolutionTree::Node *node) {
    Zobrist zob = current_state.get_zobrist();
    if (nodes_visited > config.max_nodes) {
      return save_node(node, zob, BoardValue::UNKNOWN, SolutionTree::Reason::OUT_OF_NODES, mark);
    }
    if (current_state.get_win_state()) {
      return save_node(node, zob, winner(mark), SolutionTree::Reason::WIN, mark);
    }
    if (auto has_zobrist = zobrist.find(zob); has_zobrist != zobrist.end()) {
      return save_node(node, zob, has_zobrist->second, SolutionTree::Reason::ZOBRIST, mark);
    }
    auto open_positions = current_state.get_open_positions(mark);
    if (open_positions.none()) {
      return save_node(node, zob, BoardValue::DRAW, SolutionTree::Reason::DRAW, mark);
    }
    if (auto forced = check_chaining_strategy(current_state, mark); forced.has_value()) {
      return save_node(node, zob, *forced, SolutionTree::Reason::CHAINING, mark);
    }
    if (auto forced = check_forced_win(current_state, mark, open_positions); forced.has_value()) {
      return save_node(node, zob, *forced, SolutionTree::Reason::FORCED_WIN, mark);
    }
    return {};
  }

  template<typename B>
  pair<vector<pair<int, Position>>, bag<State<N, D>>> get_children(
      const State<N, D>& current_state, Mark mark, B open_positions) {
    vector<pair<int, Position>> sorted;
    bag<State<N, D>> child_state;

    auto s = ForcingMove<N, D>(current_state);
    auto forcing = s.check(mark, open_positions);
    if (forcing.first.has_value()) {
      assert(forcing.second != mark);
      child_state.emplace_back(current_state);
      State<N, D>& cloned = *child_state.begin();
      bool game_ended = cloned.play(*forcing.first, mark);
      assert(!game_ended);
      int dummy_score = 1;
      sorted = {{dummy_score, *forcing.first}};
    } else {
      child_state.reserve(open_positions.count());
      sorted = get_sorted_positions(current_state, open_positions, mark);
      for (const auto& [score, pos] : sorted) {
        child_state.emplace_back(current_state);
        child_state.rbegin()->play(pos, mark);
      }
    }
    return {sorted, child_state};
  }

  BoardValue save_node(SolutionTree::Node *node, Zobrist node_zobrist,
      BoardValue value, SolutionTree::Reason reason, Mark mark, bool is_final = true) {
    const lock_guard lock(node_m);
    return unsafe_save_node(node, node_zobrist, value, reason, mark, is_final);
  }

  BoardValue unsafe_save_node(SolutionTree::Node *node, Zobrist node_zobrist,
      BoardValue value, SolutionTree::Reason reason, Mark mark, bool is_final = true) {
    node->reason = reason;
    node->value = value;
    node->node_final = is_final;
    if (node->is_final()) {
      zobrist[node_zobrist] = value;
    }
    if (node->parent != nullptr) {
      auto parent_turn = flip(to_turn(mark));
      auto [new_parent_value, is_final] = get_updated_parent_value(value, node->parent, parent_turn);
      if (new_parent_value.has_value()) {
        bool parent_is_final = parent_turn == Turn::X ?
            (new_parent_value == BoardValue::X_WIN) :
            (new_parent_value == BoardValue::O_WIN || new_parent_value == BoardValue::DRAW);
        auto parent_reason = parent_is_final ?
            SolutionTree::Reason::MINIMAX_EARLY : SolutionTree::Reason::MINIMAX_COMPLETE;
        unsafe_save_node(
            node->parent, node->parent->zobrist, *new_parent_value, parent_reason, flip(mark), parent_is_final);
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
    if (new_value != parent->value) {
      return {new_value, false};
    } else {
      return {{}, false};
    }
  }

  template<typename B>
  vector<pair<int, Position>> get_sorted_positions(
      const State<N, D>& current_state, const B& open, Mark mark) {
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

  void report_progress() {
    if ((nodes_visited % 1000) == 0) {
      config.debug << "id "s << nodes_visited << "\n"s;
    }
    nodes_visited++;
  }

  optional<BoardValue> check_chaining_strategy(const State<N, D>& current_state, Mark mark) {
    auto c = ChainingStrategy(current_state);
    auto pos = c.search(mark);
    static int max_visited = 0;
    if (c.visited > max_visited) {
      max_visited = c.visited;
      config.debug << "new record "s << max_visited << "\n"s;
    }
    if (pos.has_value()) {
      return winner(mark);
    }
    return {};
  }

  template<typename B>
  optional<BoardValue> check_forced_win(const State<N, D>& current_state, Mark mark, const B& open_positions) {
    auto s = ForcingMove<N, D>(current_state);
    auto forcing = s.check(mark, open_positions);
    if (forcing.first.has_value()) {
      if (forcing.second == mark) {
        return winner(mark);
      }
    }
    return {};
  }

};

#endif
