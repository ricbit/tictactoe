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

template<int N, int D, int max_nodes = 1000000,
    Outcome outcome = known_outcome<N, D>()>
class MiniMax {
 public:
  constexpr static Position board_size = BoardData<N, D>::board_size;

  explicit MiniMax(
    const State<N, D>& state,
    const BoardData<N, D>& data,
    default_random_engine& generator)
    :  state(state), data(data), generator(generator),
       nodes_visited(0),
       solution(board_size) {
    rank.reserve(board_size);
  }
  const State<N, D>& state;
  const BoardData<N, D>& data;
  default_random_engine& generator;
  int nodes_visited;
  vector<int> rank;
  SolutionTree solution;
  unordered_map<Zobrist, BoardValue> zobrist;

  optional<BoardValue> play(State<N, D>& current_state, Mark mark) {
    auto ans = play(current_state, mark,
        winner(flip(mark)), solution.get_root());
    cout << "Total nodes visited: " << nodes_visited << "\n";
    cout << "Nodes in solution tree: " << solution.get_root()->count << "\n";
    return ans;
  }

  const SolutionTree& get_solution() const {
    return solution;
  }

  optional<BoardValue> play(
      State<N, D>& current_state, Mark mark, BoardValue parent,
      SolutionTree::Node *node) {
    if (nodes_visited > max_nodes) {
      return node->value = BoardValue::UNKNOWN;
    }
    auto has_zobrist = zobrist.find(current_state.get_zobrist());
    if (has_zobrist != zobrist.end()) {
      node->value = has_zobrist->second;
      return has_zobrist->second;
    }
    auto open_positions = current_state.get_open_positions(mark);
    report_progress(open_positions);
    if (open_positions.none()) {
      zobrist[current_state.get_zobrist()] = BoardValue::DRAW;
      return node->value = BoardValue::DRAW;
    }
    if (auto forced = check_forced_move(
           current_state, mark, parent, open_positions, node);
        forced.has_value()) {
      zobrist[current_state.get_zobrist()] = *forced;
      return node->value = *forced;
    }
    vector<pair<int, Position>> sorted =
        get_sorted_positions(current_state, open_positions, mark);
    BoardValue current_best = winner(flip(mark));
    for (int rank_value = 0; const auto& [score, pos] : sorted) {
      node->children.emplace_back(
          pos, make_unique<SolutionTree::Node>(open_positions.count()));
      auto *child_node = node->get_last_child();
      State<N, D> cloned(current_state);
      bool result = cloned.play(pos, mark);
      if (result) {
        zobrist[current_state.get_zobrist()] = winner(mark);
        node->count += count_children(node);
        return node->value = winner(mark);
      } else {
        Mark flipped = flip(mark);
        rank.push_back(rank_value);
        optional<BoardValue> new_result =
            play(cloned, flipped, current_best, child_node);
        rank.pop_back();
        auto final_result = process_result(
            new_result, mark, parent, current_best);
        if (final_result.has_value()) {
          zobrist[current_state.get_zobrist()] = *final_result;
          node->count += count_children(node);
          return node->value = *final_result;
        }
      }
      rank_value++;
    }
    zobrist[current_state.get_zobrist()] = current_best;
    node->count += count_children(node);
    return node->value = current_best;
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
      BoardValue parent, BoardValue& current_best) {
    if (new_result.has_value()) {
      if (*new_result == winner(mark)) {
        return new_result;
      }
      if (*new_result == BoardValue::DRAW) {
        if (mark == Mark::O) {
          return new_result;
        }
        if (parent == BoardValue::DRAW) {
          return parent;
        } else {
          current_best = *new_result;
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

  template<typename B>
  void report_progress(const B& open_positions) {
    if ((nodes_visited % 1000) == 0) {
      cout << "id " << nodes_visited << " " << open_positions.count() << endl;
      cout << "rank ";
      for (int i : rank) {
        cout << i <<  " ";
      }
      cout << "\n";
    }
    nodes_visited++;
  }

  template<typename B>
  optional<BoardValue> check_forced_move(
      State<N, D>& current_state, Mark mark, BoardValue parent,
      const B& open_positions, SolutionTree::Node *node) {
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
    auto s = ForcingMove<N, D>(current_state);
    auto forcing = s.check(mark, open_positions);
    if (forcing.first.has_value()) {
      if (forcing.second == mark) {
        return winner(mark);
      }
      State<N, D> cloned(current_state);
      bool game_ended = cloned.play(*forcing.first, mark);
      assert(!game_ended);
      rank.push_back(-1);
      node->children.emplace_back(*forcing.first,
          make_unique<SolutionTree::Node>(1));
      auto *child_node = node->get_last_child();
      auto result = play(cloned, flip(mark), winner(flip(mark)), child_node);
      node->count += count_children(node);
      node->value = *result;
      rank.pop_back();
      return node->value;
    }
    return {};
  }
};

#endif
