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

template<int N, int D, int max_nodes = 1000000,
    Outcome outcome = known_outcome<N, D>()>
class MiniMax {
 public:
  constexpr static Position board_size = BoardData<N, D>::board_size;

  struct BoardNode {
    State<N, D>& current_state;
    Mark mark;
    SolutionTree::Node *node;
  };

  explicit MiniMax(
    const State<N, D>& state,
    const BoardData<N, D>& data)
    :  state(state), data(data),
       nodes_visited(0),
       solution(board_size) {
    rank.reserve(board_size);
  }
  const State<N, D>& state;
  const BoardData<N, D>& data;
  int nodes_visited;
  vector<int> rank;
  SolutionTree solution;
  queue<BoardNode> next;
  unordered_map<Zobrist, BoardValue, IdentityHash> zobrist;

  optional<BoardValue> play(State<N, D>& current_state, Mark mark) {
    auto ans = queue_play(BoardNode{current_state, mark, solution.get_root()});
    cout << "Total nodes visited: " << nodes_visited << "\n";
    cout << "Nodes in solution tree: " << solution.get_root()->count << "\n";
    solution.prune();
    cout << "Nodes in solution tree after pruning: "
         << solution.real_count() << "\n";
    return ans;
  }

  const SolutionTree& get_solution() const {
    return solution;
  }

  optional<BoardValue> queue_play(BoardNode root) {
    return play(root.current_state, root.mark, root.node);
    /*assert(next.empty());i
    next.push(root);
    while (!next.empty()) {
      BoardNode board_node = next.front();
      next.pop();
      play(board_node.current_state, board_node.mark, board_node.node);
    }*/
  }

  optional<BoardValue> check_terminal_node(State<N, D>& current_state, Mark mark, SolutionTree::Node *node) {
    report_progress();
    Zobrist zob = current_state.get_zobrist();
    if (nodes_visited > max_nodes) {
      return save_node(node, zob, 0, BoardValue::UNKNOWN, SolutionTree::Reason::OUT_OF_NODES);
    }
    if (current_state.get_win_state()) {
      return save_node(node, zob, count_children(node), winner(mark), SolutionTree::Reason::WIN);
    }
    auto has_zobrist = zobrist.find(zob);
    if (has_zobrist != zobrist.end()) {
      return save_node(node, zob, 0, has_zobrist->second, SolutionTree::Reason::ZOBRIST);
    }
    auto open_positions = current_state.get_open_positions(mark);
    if (open_positions.none()) {
      return save_node(node, zob, 0, BoardValue::DRAW, SolutionTree::Reason::DRAW);
    }
    if (auto forced = check_chaining_strategy(current_state, mark, open_positions, node); forced.has_value()) {
      return save_node(node, zob, 0, *forced, SolutionTree::Reason::CHAINING);
    }
    if (auto forced = check_forced_win(current_state, mark, open_positions, node); forced.has_value()) {
      return save_node(node, zob, 0, *forced, SolutionTree::Reason::FORCED_WIN);
    }
    return {};
  }

  optional<BoardValue> play(State<N, D>& current_state, Mark mark, SolutionTree::Node *node) {
    auto terminal_node = check_terminal_node(current_state, mark, node);
    if (terminal_node.has_value()) {
      return terminal_node;
    }
    auto open_positions = current_state.get_open_positions(mark);
    node->value = winner(flip(mark));

    auto [sorted, child_state] = get_children(current_state, mark, open_positions);
    for (int rank_value = 0; const auto& [score, pos] : sorted) {
      node->children.emplace_back(pos, make_unique<SolutionTree::Node>(node, open_positions.count()));
      auto *child_node = node->get_last_child();
      State<N, D>& cloned = child_state[rank_value];
      child_node->zobrist = cloned.get_zobrist();
      Mark flipped = flip(mark);
      rank.push_back(rank_value);
      optional<BoardValue> new_result = play(cloned, flipped, child_node);
      rank.pop_back();
      auto final_result = process_result(new_result, mark, node->value, node);
      if (final_result.has_value()) {
        return save_node(node, current_state.get_zobrist(),
            count_children(node), *final_result, SolutionTree::Reason::MINIMAX_EARLY);
      }
      rank_value++;
    }
    return save_node(node, current_state.get_zobrist(),
        count_children(node), node->value, SolutionTree::Reason::MINIMAX_COMPLETE);
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
      State<N, D> cloned(current_state);
      bool game_ended = cloned.play(*forcing.first, mark);
      assert(!game_ended);
      int dummy_score = 1;
      sorted = {{dummy_score, *forcing.first}};
      child_state.push_back(cloned);
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
      int children_count, BoardValue value, SolutionTree::Reason reason) {
    zobrist[node_zobrist] = value;
    node->count += children_count;
    node->reason = reason;
    return node->value = value;
  }

  BoardValue winner(Mark mark) {
    return mark == Mark::X ? BoardValue::X_WIN : BoardValue::O_WIN;
  }

  int count_children(SolutionTree::Node *parent) {
    return accumulate(begin(parent->children), end(parent->children), 0,
      [](auto a, auto& b) {
        return a + b.second->count;
      });
  }

  optional<BoardValue> process_result(
      optional<BoardValue> new_result, Mark mark,
      BoardValue& current_best, SolutionTree::Node *node) {
    if (new_result.has_value()) {
      if (*new_result == winner(mark)) {
        return new_result;
      }
      if (*new_result == BoardValue::DRAW) {
        if (mark == Mark::O) {
          return new_result;
        }
        if (node->get_parent_value() == BoardValue::DRAW) {
          return node->get_parent_value();
        } else {
          node->value = *new_result;
        }
      } else if (*new_result == BoardValue::UNKNOWN) {
        return new_result;
      }
    }
    return {};
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
      cout << "id " << nodes_visited << endl;
      cout << "rank ";
      for (int i : rank) {
        cout << i <<  " ";
      }
      cout << "\n";
    }
    nodes_visited++;
  }

  template<typename B>
  optional<BoardValue> check_chaining_strategy(
      State<N, D>& current_state, Mark mark, const B& open_positions, SolutionTree::Node *node) {
    auto c = ChainingStrategy(current_state);
    auto pos = c.search(mark);
    static int max_visited = 0;
    if (c.visited > max_visited) {
      max_visited = c.visited;
      cout << "new record " << max_visited << endl;
    }
    if (pos.has_value()) {
      return winner(mark);
    }
    return {};
  }

  template<typename B>
  optional<BoardValue> check_forced_win(
      State<N, D>& current_state, Mark mark, const B& open_positions, SolutionTree::Node *node) {
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
