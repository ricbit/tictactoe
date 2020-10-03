#include "minimax.hh"
#include "node.hh"
#include "gtest/gtest.h"

namespace {

TEST(GeometryTest, CorrectNumberOfLines) {
  EXPECT_EQ(109, (Geometry<5, 3>::line_size));
  EXPECT_EQ(76, (Geometry<4, 3>::line_size));
  EXPECT_EQ(49, (Geometry<3, 3>::line_size));
}

TEST(GeometryTest, Decode) {
  Geometry<4, 2> data;
  auto actual = data.decode(12_pos);
  auto expected = Geometry<4, 2>::SideArray{0_side, 3_side};
  EXPECT_EQ(expected, actual);
}

TEST(GeometryTest, DecodeEncode) {
  Geometry<4, 2> data;
  auto actual = data.decode(12_pos);
  EXPECT_EQ(12_pos, data.encode(actual));
}

TEST(GeometryTest, EncodeDecode) {
  Geometry<4, 2> data;
  auto expected = Geometry<4, 2>::SideArray{0_side, 3_side};
  EXPECT_EQ(expected, data.decode(data.encode(expected)));
}

TEST(BoardValueTest, StarshipComparison) {
  EXPECT_LT(BoardValue::X_WIN, BoardValue::O_WIN);
  EXPECT_EQ(BoardValue::DRAW, BoardValue::DRAW);
  EXPECT_GT(BoardValue::O_WIN, BoardValue::DRAW);
}

TEST(SymmetryTest, CorrectNumberOfSymmetries) {
  Geometry<5, 3> geom53;
  EXPECT_EQ(192u, Symmetry(geom53).symmetries().size());
  Geometry<3, 3> geom33;
  EXPECT_EQ(48u, Symmetry(geom33).symmetries().size());
  Geometry<3, 2> geom32;
  EXPECT_EQ(8u, Symmetry(geom32).symmetries().size());
}

TEST(SymmeTrieTest, CheckTrieInvariant) {
  Geometry<3, 3> geom;
  Symmetry sym(geom);
  SymmeTrie trie(sym);
  Position board_size = Geometry<3, 3>::board_size;
  for (NodeLine line = 0_node; line < trie.size(); ++line) {
    for (Position pos = 0_pos; pos < board_size; ++pos) {
      auto before = trie.mask(line, pos);
      for (Position next = 0_pos; next < board_size; ++next) {
        auto after = trie.mask(trie.next(line, pos), pos);
        EXPECT_EQ(after, before & after);
      }
    }
  }
}

TEST(SymmeTrieTest, PlayingInTheMiddleNeverChangesSymmetry) {
  Geometry<3, 3> geom;
  Symmetry sym(geom);
  SymmeTrie trie(sym);
  Position board_size = Geometry<3, 3>::board_size;
  const Position middle = geom.encode({1_side, 1_side, 1_side});
  for (NodeLine line = 0_node; line < trie.size(); ++line) {
    for (Position pos = 0_pos; pos < board_size; ++pos) {
      auto before = trie.mask(line, pos);
      auto after = trie.mask(trie.next(line, middle), pos);
      EXPECT_EQ(before, after);
    }
  }
}

TEST(SymmeTrieTest, CheckIdentityNode) {
  Geometry<3, 3> geom;
  Symmetry sym(geom);
  SymmeTrie trie(sym);
  EXPECT_FALSE(trie.is_identity(0_node));
  NodeLine current = 0_node;
  Position board_size = Geometry<3, 3>::board_size;
  for (Position pos = 0_pos; pos < board_size; ++pos) {
    current = trie.next(current, pos);
  }
  EXPECT_TRUE(trie.is_identity(current));
}

TEST(StateTest, CorrectNumberOfOpeningPositions) {
  EXPECT_EQ(2u, (State(BoardData<4, 3>()).get_open_positions(Mark::X).count()));
  EXPECT_EQ(6u, (State(BoardData<5, 3>()).get_open_positions(Mark::X).count()));
}

template<typename State>
optional<Position> get_first_open_position(State& state) {
  const auto& open_positions = state.get_open_positions(Mark::X);
  for (Position pos = 0_pos; pos < state.board_size; ++pos) {
    if (open_positions[pos]) {
      return pos;
    }
  }
  optional<Position> empty_position = {};
  return empty_position;
}

TEST(StateTest, CopiedStateDoesntChangeTheOriginal) {
  BoardData<5, 3> data;
  State original(data);
  State<5, 3> cloned(original);
  auto pos = get_first_open_position(cloned);
  EXPECT_TRUE(pos.has_value());
  cloned.play(*pos, Mark::X);
  EXPECT_NE(
      cloned.get_open_positions(Mark::X).count(),
      original.get_open_positions(Mark::X).count());
}

TEST(StateTest, OpenPositionsOnDefensiveMoveIn33) {
  BoardData<3, 3> data;
  State state(data);
  state.play({1_side, 0_side, 0_side}, Mark::X);
  state.play({1_side, 1_side, 1_side}, Mark::O);
  state.play({1_side, 1_side, 0_side}, Mark::X);
  auto open = state.get_open_positions(Mark::O);
  EXPECT_TRUE(open[data.encode({1_side, 2_side, 0_side})]);
}

TEST(StateTest, DefensiveMoveIn33DifferentOrder) {
  BoardData<3, 3> data;
  State state(data);
  state.play({1_side, 1_side, 0_side}, Mark::X);
  state.play({1_side, 1_side, 1_side}, Mark::O);
  state.play({1_side, 0_side, 0_side}, Mark::X);
  auto open = state.get_open_positions(Mark::O);
  EXPECT_TRUE(open[data.encode({1_side, 2_side, 0_side})]);
}

TEST(TrackingListTest, ProperlyBuilt) {
  TrackingList<5, 3> tracking;
  int count = 0;
  for ([[maybe_unused]] auto p : tracking) {
    count++;
  }
  EXPECT_EQ(125, count);
}

TEST(TrackingListTest, IterateElements) {
  TrackingList<3, 1> tracking;
  array expected{0_pos, 1_pos, 2_pos};
  for (int i = 0; auto value : tracking) {
    EXPECT_EQ(expected[i++], value);
  }
}

TEST(TrackingListTest, DeleteElements) {
  TrackingList<5, 1> tracking;
  array expected{1_pos, 3_pos};
  tracking.remove(0_pos);
  tracking.remove(2_pos);
  tracking.remove(4_pos);
  for (int i = 0; auto value : tracking) {
    EXPECT_EQ(expected[i++], value);
  }
}

TEST(TrackingListTest, CheckElements) {
  TrackingList<5, 1> tracking;
  array expected{false, true, false, true, false};
  tracking.remove(0_pos);
  tracking.remove(2_pos);
  tracking.remove(4_pos);
  for (Position pos = 0_pos; pos < 5_pos; ++pos) {
    EXPECT_EQ(expected[pos], tracking.check(pos));
  }
}

TEST(TrackingListTest, IsCopyable) {
  TrackingList<5, 1> tracking;
  array original{false, true, false, true, false};
  tracking.remove(0_pos);
  tracking.remove(2_pos);
  tracking.remove(4_pos);
  TrackingList<5, 1> clone(tracking);
  array copied{false, true, false, false, false};
  clone.remove(3_pos);
  for (Position pos = 0_pos; pos < 5_pos; ++pos) {
    EXPECT_EQ(original[pos], tracking.check(pos));
    EXPECT_EQ(copied[pos], clone.check(pos));
  }
}


TEST(TrackingListTest, EmptyWorks) {
  TrackingList<3, 1> tracking;
  tracking.remove(0_pos);
  tracking.remove(1_pos);
  tracking.remove(2_pos);
  int count = 0;
  for ([[maybe_unused]] auto p : tracking) {
    count++;
  }
  EXPECT_EQ(0, count);
}

TEST(ElevatorTest, StartAtLevelZero) {
  Elevator<5, 3> elevator;
  for (Line line = 0_line; line < BoardData<5, 3>::line_size; ++line) {
    MarkCount count = elevator[line];
    EXPECT_EQ(0_mcount, count);
  }
}

TEST(ElevatorTest, IncrementAndDecrement) {
  Elevator<5, 3> elevator;
  Line line = 50_line;
  EXPECT_EQ(1_mcount, elevator[line] += Mark::X);
  EXPECT_EQ(2_mcount, elevator[line] += Mark::X);
  EXPECT_EQ(1_mcount, elevator[line] -= Mark::X);
  EXPECT_EQ(0_mcount, elevator[line] -= Mark::X);
}

TEST(ElevatorTest, IterateFloorZero) {
  Elevator<3, 2> elevator;
  vector<Line> expected(8);
  iota(begin(expected), end(expected), 0_line);
  vector<Line> actual;
  for (auto value : elevator.all(0_mcount, Mark::empty)) {
    actual.push_back(value);
  }
  sort(begin(actual), end(actual));
  EXPECT_EQ(expected, actual);
}

TEST(ElevatorTest, IterateFloorOne) {
  Elevator<3, 2> elevator;
  elevator[5_line] += Mark::X;
  elevator[2_line] += Mark::X;
  vector<Line> expected{2_line, 5_line};
  vector<Line> actual;
  for (auto value : elevator.all(1_mcount, Mark::X)) {
    actual.push_back(value);
  }
  sort(begin(actual), end(actual));
  EXPECT_EQ(expected, actual);
}


TEST(ElevatorTest, IterateFloorTwo) {
  Elevator<3, 2> elevator;
  elevator[5_line] += Mark::X;
  elevator[2_line] += Mark::X;
  elevator[2_line] += Mark::X;
  elevator[3_line] += Mark::X;
  elevator[5_line] += Mark::X;
  elevator[2_line] -= Mark::X;
  elevator[3_line] += Mark::X;
  elevator[5_line] += Mark::X;
  elevator[5_line] -= Mark::X;
  vector<Line> expected{3_line, 5_line};
  vector<Line> actual;
  for (auto value : elevator.all(2_mcount, Mark::X)) {
    actual.push_back(value);
  }
  sort(begin(actual), end(actual));
  EXPECT_EQ(expected, actual);
}

TEST(ElevatorTest, IterateFloorThree) {
  Elevator<4, 2> elevator;
  elevator[5_line] += Mark::X;
  elevator[5_line] += Mark::X;
  elevator[5_line] += Mark::X;
  vector<Line> expected{5_line};
  vector<Line> actual;
  for (auto value : elevator.all(3_mcount, Mark::X)) {
    actual.push_back(value);
  }
  sort(begin(actual), end(actual));
  EXPECT_EQ(expected, actual);
}

TEST(ElevatorTest, IterateEmptyFloor) {
  Elevator<3, 2> elevator;
  int count = 0;
  for ([[maybe_unused]] auto value : elevator.all(2_mcount, Mark::X)) {
    count++;
  }
  EXPECT_EQ(0, count);
}

TEST(ElevatorTest, CopyPreservesOriginal) {
  Elevator<3, 2> elevator;
  elevator[4_line] += Mark::X;
  elevator[4_line] += Mark::X;
  Elevator<3, 2> other(elevator);
  other[4_line] -= Mark::X;
  EXPECT_EQ(2_mcount, MarkCount{elevator[4_line]});
  EXPECT_EQ(1_mcount, MarkCount{other[4_line]});
}

TEST(ElevatorTest, IterateDifferentMarks) {
  Elevator<3, 2> elevator;
  elevator[2_line] += Mark::X;
  elevator[2_line] += Mark::X;
  elevator[3_line] += Mark::O;
  elevator[3_line] += Mark::O;
  elevator[5_line] += Mark::X;
  elevator[5_line] += Mark::X;
  elevator[5_line] += Mark::X;
  elevator[4_line] += Mark::O;
  elevator[4_line] += Mark::X;
  vector<Line> expected{2_line};
  vector<Line> actual;
  for (auto value : elevator.all(2_mcount, Mark::X)) {
    actual.push_back(value);
  }
  sort(begin(actual), end(actual));
  EXPECT_EQ(expected, actual);
}

TEST(ElevatorTest, CheckLine) {
  Elevator<3, 2> elevator;
  EXPECT_TRUE(elevator.check(2_line, 0_mcount, Mark::empty));
  EXPECT_FALSE(elevator.check(2_line, 1_mcount, Mark::X));
  EXPECT_FALSE(elevator.check(2_line, 2_mcount, Mark::X));
  EXPECT_FALSE(elevator.check(2_line, 3_mcount, Mark::both));
  elevator[2_line] += Mark::X;
  EXPECT_FALSE(elevator.check(2_line, 0_mcount, Mark::empty));
  EXPECT_TRUE(elevator.check(2_line, 1_mcount, Mark::X));
  EXPECT_FALSE(elevator.check(2_line, 2_mcount, Mark::X));
  EXPECT_FALSE(elevator.check(2_line, 3_mcount, Mark::both));
  elevator[2_line] += Mark::X;
  EXPECT_FALSE(elevator.check(2_line, 0_mcount, Mark::empty));
  EXPECT_FALSE(elevator.check(2_line, 1_mcount, Mark::X));
  EXPECT_TRUE(elevator.check(2_line, 2_mcount, Mark::X));
  EXPECT_FALSE(elevator.check(2_line, 3_mcount, Mark::both));
  elevator[2_line] += Mark::O;
  EXPECT_FALSE(elevator.check(2_line, 0_mcount, Mark::empty));
  EXPECT_FALSE(elevator.check(2_line, 1_mcount, Mark::X));
  EXPECT_FALSE(elevator.check(2_line, 2_mcount, Mark::X));
  EXPECT_TRUE(elevator.check(2_line, 3_mcount, Mark::both));
}

TEST(ElevatorTest, Empty) {
  Elevator<3, 2> elevator;
  EXPECT_TRUE(elevator.empty(2_mcount, Mark::X));
  elevator[5_line] += Mark::X;
  EXPECT_TRUE(elevator.empty(2_mcount, Mark::X));
  elevator[5_line] += Mark::X;
  EXPECT_FALSE(elevator.empty(2_mcount, Mark::X));
  elevator[5_line] += Mark::X;
  EXPECT_TRUE(elevator.empty(2_mcount, Mark::X));
}

TEST(ElevatorTest, One) {
  Elevator<3, 2> elevator;
  EXPECT_FALSE(elevator.one(2_mcount, Mark::X));
  elevator[5_line] += Mark::X;
  EXPECT_FALSE(elevator.one(2_mcount, Mark::X));
  elevator[5_line] += Mark::X;
  EXPECT_TRUE(elevator.one(2_mcount, Mark::X));
  elevator[3_line] += Mark::X;
  elevator[3_line] += Mark::X;
  EXPECT_FALSE(elevator.one(2_mcount, Mark::X));
  elevator[5_line] += Mark::X;
  EXPECT_TRUE(elevator.one(2_mcount, Mark::X));
}

TEST(ChainingStrategyTest, LineOfX) {
  BoardData<3, 2> data;
  State state(data);
  state.play({0_side, 0_side}, Mark::X);
  state.play({0_side, 1_side}, Mark::X);
  ChainingStrategy strat(state);
  EXPECT_EQ(
      data.encode({0_side, 2_side}),
      *strat(Mark::X, state.get_open_positions(Mark::X)));
  EXPECT_FALSE(
      strat(Mark::O, state.get_open_positions(Mark::O)).has_value());
}

TEST(ChainingStrategyTest, CrossOfX) {
  BoardData<4, 2> data;
  State state(data);
  state.play({0_side, 0_side}, Mark::X);
  state.play({0_side, 1_side}, Mark::X);
  state.play({2_side, 3_side}, Mark::X);
  state.play({3_side, 3_side}, Mark::X);
  ChainingStrategy strat(state);
  EXPECT_TRUE(
      strat(Mark::X, state.get_open_positions(Mark::X)).has_value());
  EXPECT_FALSE(
      strat(Mark::O, state.get_open_positions(Mark::O)).has_value());
}

TEST(ChainingStrategyTest, DiagonalXO) {
  BoardData<3, 2> data;
  State state(data);
  state.play({0_side, 0_side}, Mark::X);
  state.play({2_side, 2_side}, Mark::O);
  ChainingStrategy strat(state);
  EXPECT_TRUE(
      strat(Mark::X, state.get_open_positions(Mark::X)).has_value());
}

TEST(ChainingStrategyTest, BlockedCrossOfX) {
  BoardData<4, 2> data;
  State state(data);
  state.play({0_side, 0_side}, Mark::X);
  state.play({0_side, 1_side}, Mark::X);
  state.play({2_side, 3_side}, Mark::X);
  state.play({3_side, 3_side}, Mark::X);
  state.play({3_side, 0_side}, Mark::O);
  state.play({2_side, 1_side}, Mark::O);
  state.play({1_side, 2_side}, Mark::O);
  ChainingStrategy strat(state);
  EXPECT_FALSE(
      strat(Mark::X, state.get_open_positions(Mark::X)).has_value());
  EXPECT_TRUE(
      strat(Mark::O, state.get_open_positions(Mark::O)).has_value());
}

TEST(ForcingMoveTest, CheckDefensiveMoveUsingOperator) {
  BoardData<3, 2> data;
  State state(data);
  state.play({0_side, 0_side}, Mark::X);
  state.play({0_side, 2_side}, Mark::O);
  state.play({0_side, 1_side}, Mark::X);
  state.play({1_side, 1_side}, Mark::O);
  auto open = state.get_open_positions(Mark::X);
  ForcingMove f(state);
  auto pos = f(Mark::X, open);
  EXPECT_TRUE(pos);
  EXPECT_EQ(data.encode({2_side, 0_side}), *pos);
}

TEST(ForcingMoveTest, CheckDefensiveMoveUsingCheck) {
  BoardData<3, 2> data;
  State state(data);
  state.play({0_side, 0_side}, Mark::X);
  state.play({0_side, 2_side}, Mark::O);
  state.play({0_side, 1_side}, Mark::X);
  state.play({1_side, 1_side}, Mark::O);
  auto open = state.get_open_positions(Mark::X);
  ForcingMove f(state);
  auto pos = f.check(Mark::X, open);
  EXPECT_TRUE(pos.first);
  EXPECT_EQ(data.encode({2_side, 0_side}), *pos.first);
  EXPECT_EQ(Mark::O, pos.second);
}

TEST(ForcingMoveTest, CheckDefensiveMoveIn33) {
  BoardData<3, 3> data;
  State state(data);
  state.play({1_side, 0_side, 0_side}, Mark::X);
  state.play({1_side, 1_side, 1_side}, Mark::O);
  state.play({1_side, 1_side, 0_side}, Mark::X);
  auto open = state.get_open_positions(Mark::O);
  ForcingMove f(state);
  auto pos = f.check(Mark::O, open);
  EXPECT_TRUE(pos.first);
  EXPECT_EQ(data.encode({1_side, 2_side, 0_side}), *pos.first);
  EXPECT_EQ(Mark::X, pos.second);
}

struct GetParentValueTestValues {
  BoardValue child;
  BoardValue parent;
  Turn turn;
  pair<optional<BoardValue>, bool> result;
};

struct TestingTree {
  BoardValue value;
  bool is_final;
  vector<pair<BoardValue, bool>> children;
};

template<int M>
struct TestingNodes {
  vector<Node<M>> nodes;
  Node<M> *root;
};

template<int M>
TestingNodes<M> build_tree(TestingTree tree) {
  TestingNodes<M> nodes;
  Turn dont_care = Turn::X;
  nodes.nodes.reserve(1 + tree.children.size());
  Node<M> *parent = &nodes.nodes.emplace_back(nullptr, dont_care, tree.children.size());
  parent->set_value(tree.value);
  parent->set_is_final(tree.is_final);
  for_each(begin(tree.children), end(tree.children), [&](const auto& child_values) {
    const auto& [value, is_final] = child_values;
    auto& child = nodes.nodes.emplace_back(parent, dont_care, 0);
    parent->emplace_child(0_pos, &*nodes.nodes.rbegin());
    child.set_value(value);
    child.set_is_final(is_final);
  });
  nodes.root = &*nodes.nodes.begin();
  return nodes;
}

template<typename T>
auto get_parent_checker(T& minimax) {
  return [&](BoardValue child_value, BoardValue parent_value, Turn turn, pair<optional<BoardValue>, bool> result) {
    auto solution = build_tree<T::M>({parent_value, false, {
        {parent_value, true},
        {child_value, true},
        {BoardValue::UNKNOWN, false}
    }});
    auto& parent = solution.root;
    return result == minimax.get_updated_parent_value(child_value, parent, turn);
  };
}

/*TEST(MiniMaxTest, GetParentValue) {
  BoardData<3, 2> data;
  State state(data);
  auto minimax = MiniMax(state, data);
  vector<GetParentValueTestValues> test_values = {
    {BoardValue::X_WIN, BoardValue::UNKNOWN, Turn::X, {BoardValue::X_WIN, true}},
    {BoardValue::O_WIN, BoardValue::UNKNOWN, Turn::X, {BoardValue::O_WIN, false}},
    {BoardValue::DRAW,  BoardValue::UNKNOWN, Turn::X, {BoardValue::DRAW,  false}},
    {BoardValue::X_WIN, BoardValue::X_WIN,   Turn::X, {{}, true}},
    {BoardValue::O_WIN, BoardValue::X_WIN,   Turn::X, {{}, true}},
    {BoardValue::DRAW,  BoardValue::X_WIN,   Turn::X, {{}, true}},
    {BoardValue::X_WIN, BoardValue::O_WIN,   Turn::X, {BoardValue::X_WIN, true}},
    {BoardValue::O_WIN, BoardValue::O_WIN,   Turn::X, {{}, false}},
    {BoardValue::DRAW,  BoardValue::O_WIN,   Turn::X, {BoardValue::DRAW,  false}},
    {BoardValue::X_WIN, BoardValue::DRAW,    Turn::X, {BoardValue::X_WIN, true}},
    {BoardValue::O_WIN, BoardValue::DRAW,    Turn::X, {{}, false}},
    {BoardValue::DRAW,  BoardValue::DRAW,    Turn::X, {{}, false}},
    {BoardValue::X_WIN, BoardValue::UNKNOWN, Turn::O, {BoardValue::X_WIN, false}},
    {BoardValue::O_WIN, BoardValue::UNKNOWN, Turn::O, {BoardValue::O_WIN, true}},
    {BoardValue::DRAW,  BoardValue::UNKNOWN, Turn::O, {BoardValue::DRAW,  true}},
    {BoardValue::X_WIN, BoardValue::X_WIN,   Turn::O, {{}, false}},
    {BoardValue::O_WIN, BoardValue::X_WIN,   Turn::O, {BoardValue::O_WIN, true}},
    {BoardValue::DRAW,  BoardValue::X_WIN,   Turn::O, {BoardValue::DRAW,  true}},
    {BoardValue::X_WIN, BoardValue::O_WIN,   Turn::O, {{}, true}},
    {BoardValue::O_WIN, BoardValue::O_WIN,   Turn::O, {{}, true}},
    {BoardValue::DRAW,  BoardValue::O_WIN,   Turn::O, {{}, true}},
    {BoardValue::X_WIN, BoardValue::DRAW,    Turn::O, {{}, true}},
    {BoardValue::O_WIN, BoardValue::DRAW,    Turn::O, {BoardValue::O_WIN, true}},
    {BoardValue::DRAW,  BoardValue::DRAW,    Turn::O, {{}, true}},
  };
  for_each(begin(test_values), end(test_values), [&](auto& value) {
    EXPECT_PRED4(get_parent_checker(minimax), value.child, value.parent, value.turn, value.result);
  });
}*/

/*TEST(MiniMaxTest, GetParentValueKeepValueChangeIsFinal) {
  BoardData<3, 2> data;
  State state(data);
  auto minimax = MiniMax(state, data);
  auto solution = build_tree<decltype(minimax)::M>({BoardValue::DRAW, false, {
      {BoardValue::DRAW, true},
      {BoardValue::DRAW, true}
  }});
  auto& parent = solution.root;
  auto [new_value, is_final] = minimax.get_updated_parent_value(BoardValue::DRAW, parent, Turn::X);
  EXPECT_FALSE(new_value.has_value());
  EXPECT_TRUE(is_final);
}

TEST(MiniMaxTest, GetParentValueOneDrawOneUnknown) {
  BoardData<3, 2> data;
  State state(data);
  auto minimax = MiniMax(state, data);
  auto solution = build_tree<decltype(minimax)::M>({BoardValue::UNKNOWN, false, {
      {BoardValue::UNKNOWN, false},
      {BoardValue::DRAW, true}
  }});
  auto& parent = solution.root;
  auto [new_value, is_final] = minimax.get_updated_parent_value(BoardValue::DRAW, parent, Turn::X);
  EXPECT_EQ(BoardValue::DRAW, *new_value);
  EXPECT_FALSE(is_final);
}

TEST(MiniMaxTest, GetParentValueOneDrawNotFinalOneUnknownTurnO) {
  BoardData<3, 2> data;
  State state(data);
  auto minimax = MiniMax(state, data);
  auto solution = build_tree<decltype(minimax)::M>({BoardValue::UNKNOWN, false, {
      {BoardValue::UNKNOWN, false},
      {BoardValue::DRAW, false}
  }});
  auto& parent = solution.root;
  auto [new_value, is_final] = minimax.get_updated_parent_value(BoardValue::DRAW, parent, Turn::O);
  EXPECT_EQ(BoardValue::DRAW, *new_value);
  EXPECT_FALSE(is_final);
}

TEST(MiniMaxTest, GetParentValueMinimaxCompleteIsFinalByExaustion) {
  BoardData<3, 2> data;
  State state(data);
  auto minimax = MiniMax(state, data);
  auto solution = build_tree<decltype(minimax)::M>({BoardValue::UNKNOWN, false, {
      {BoardValue::DRAW, true},
      {BoardValue::DRAW, true}
  }});
  auto& parent = solution.root;
  auto [new_value, is_final] = minimax.get_updated_parent_value(BoardValue::DRAW, parent, Turn::X);
  EXPECT_EQ(BoardValue::DRAW, *new_value);
  EXPECT_TRUE(is_final);
}

TEST(MiniMaxTest, GetParentValueAnyOfOWinAndDrawAreEquivalentForO) {
  BoardData<3, 2> data;
  State state(data);
  auto minimax = MiniMax(state, data);
  auto solution = build_tree<decltype(minimax)::M>({BoardValue::O_WIN, false, {
      {BoardValue::O_WIN, false},
      {BoardValue::DRAW, true}
  }});
  auto& parent = solution.root;
  auto [new_value, is_final] = minimax.get_updated_parent_value(BoardValue::DRAW, parent, Turn::O);
  EXPECT_EQ(BoardValue::DRAW, *new_value);
  EXPECT_TRUE(is_final);
}*/

TEST(MiniMaxTest, Check32DFS) {
  BoardData<3, 2> data;
  State state(data);
  auto minimax = MiniMax(state, data);
  auto result = minimax.play(state, Turn::X);
  EXPECT_EQ(BoardValue::DRAW, *result);
  EXPECT_TRUE(minimax.get_solution().validate());
}

TEST(MiniMaxTest, Check33DFS) {
  BoardData<3, 3> data;
  State state(data);
  auto minimax = MiniMax(state, data);
  auto result = minimax.play(state, Turn::X);
  EXPECT_EQ(BoardValue::X_WIN, *result);
  EXPECT_TRUE(minimax.get_solution().validate());
}

TEST(MiniMaxTest, Check32PNSearch) {
  BoardData<3, 2> data;
  State state(data);
  auto minimax = MiniMax<3, 2, PNSearch<3, 2, MiniMax<3, 2>::M>>(state, data);
  auto result = minimax.play(state, Turn::X);
  EXPECT_EQ(BoardValue::DRAW, *result);
  EXPECT_TRUE(minimax.get_solution().validate());
}

TEST(MiniMaxTest, Check33PNSearch) {
  BoardData<3, 3> data;
  State state(data);
  auto minimax = MiniMax<3, 3, PNSearch<3, 3, MiniMax<3, 2>::M>>(state, data);
  auto result = minimax.play(state, Turn::X);
  EXPECT_EQ(BoardValue::X_WIN, *result);
  EXPECT_TRUE(minimax.get_solution().validate());
}

struct ConfigBFSOne {
  constexpr static NodeCount max_visited = 1_nc;
  constexpr static NodeCount max_created = 100_nc;
  DummyCout debug;
  bool should_log_evolution = false;
  bool should_prune = false;
};

TEST(MiniMaxTest, CheckOneNodeOfBFS) {
  BoardData<3, 2> data;
  State state(data);
  auto minimax = MiniMax<3, 2, BFS<3, 2, ConfigBFSOne::max_created>, ConfigBFSOne>(state, data);
  minimax.play(state, Turn::X);
  auto& solution = minimax.get_solution();
  EXPECT_EQ(4, solution.real_count());
  auto first_node = solution.get_root()->get_children().begin()->second;
  EXPECT_EQ(Turn::X, solution.get_root()->get_turn());
  EXPECT_EQ(Turn::O, first_node->get_turn());
}

struct ConfigBFSMax {
  constexpr static NodeCount max_visited = 1'000'000_nc;
  constexpr static NodeCount max_created = 100_nc;
  DummyCout debug;
  bool should_log_evolution = false;
  bool should_prune = false;
};

/*TEST(MiniMaxTest, CheckMaxCreated) {
  BoardData<3, 2> data;
  State state(data);
  auto minimax = MiniMax<3, 2, BFS<3, 2, ConfigBFSMax::max_created>, ConfigBFSMax>(state, data);
  minimax.play(state, Turn::X);
  auto& solution = minimax.get_solution();
  EXPECT_EQ(100, solution.real_count());
}*/

/*TEST(MiniMaxTest, WriteOutputFile) {
  BoardData<3, 2> data;
  State state(data);
  auto minimax = MiniMax<3, 2, BFS<3, 2, ConfigBFSMax::max_created>, ConfigBFSMax>(state, data);
  minimax.play(state, Turn::X);
  auto& solution = minimax.get_solution();
  solution.update_count();
  solution.dump(data, "/dev/null");
}*/

template<int M, typename MM>
bool validate_all_parents(const Node<M> *parent, MM& minimax) {
  if (!parent->has_children()) {
    return true;
  }
  auto children = parent->get_children();
  for (const auto& child_pair : children) {
    const auto child_node = child_pair.second;
    if (child_node->get_parent() != parent) {
      auto child_state = child_node->rebuild_state(minimax.data);
      if (child_node != minimax.zobrist[child_state.get_zobrist()]) {
        return false;
      }
    }
  }
  return all_of(begin(children), end(children), [&](const auto& child_pair) {
    return validate_all_parents<M>(child_pair.second, minimax);
  });
}

TEST(SolutionTreeTest, ValidateParents) {
  BoardData<3, 2> data;
  State state(data);
  auto minimax = MiniMax(state, data);
  minimax.play(state, Turn::X);
  auto root = minimax.get_solution().get_root();
  EXPECT_TRUE(validate_all_parents<decltype(minimax)::M>(root, minimax));
}

TEST(SolutionDagTest, CreateSolutionDag) {
  BoardData<3, 2> data;
  State state(data);
  using namespace node;
  NodeIndex max_nodes = 10_nind;
  SolutionDag solution(data, max_nodes);
  node::NodeP root = solution.get_root();
  EXPECT_EQ(0u, solution.get_parents(root).size());
  EXPECT_FALSE(solution.has_parent(root));
  EXPECT_EQ(3_cind, solution.children_size(root));
}

TEST(SolutionDagTest, GetTurnOnRoot) {
  BoardData<3, 2> data;
  State state(data);
  using namespace node;
  NodeIndex max_nodes = 10_nind;
  SolutionDag solution(data, max_nodes);
  const node::NodeP root = solution.get_root();
  EXPECT_EQ(Turn::X, solution.get_turn(root));
}

TEST(SolutionDagTest, GetPositionsOnRoot) {
  BoardData<3, 2> data;
  State state(data);
  using namespace node;
  NodeIndex max_nodes = 10_nind;
  SolutionDag solution(data, max_nodes);
  node::NodeP root = solution.get_root();
  const auto positions = solution.get_positions(root);
  bag<Position> expected = {0_pos, 1_pos, 4_pos};
  EXPECT_EQ(expected, positions);
}

TEST(SolutionDagTest, GetChildOnRoot) {
  BoardData<3, 2> data;
  State state(data);
  using namespace node;
  NodeIndex max_nodes = 10_nind;
  SolutionDag solution(data, max_nodes);
  node::NodeP root = solution.get_root();
  auto child = solution.get_child(Child{root, 0_cind});
  EXPECT_EQ(1u, solution.get_parents(child).size());
  EXPECT_EQ(root, solution.get_parents(child)[0_pind]);
  EXPECT_EQ(child, solution.get_child(Child{root, 0_cind}));
  EXPECT_NE(root, child);
  EXPECT_EQ(Turn::O, solution.get_turn(child));
}

TEST(SolutionDagTest, GetChildChaining33) {
  BoardData<3, 3> data;
  State state(data);
  using namespace node;
  NodeIndex max_nodes = 10_nind;
  SolutionDag solution(data, max_nodes);
  node::NodeP root = solution.get_root();
  EXPECT_EQ(4u, solution.children_size(root));
  auto variation = solution.get_variation(bag<Position>{13_pos, 0_pos});
  EXPECT_EQ(0u, solution.children_size(variation));
  EXPECT_EQ(0u, solution.get_positions(variation).size());
  EXPECT_EQ(Turn::X, solution.get_turn(variation));
}

TEST(SolutionDagTest, GetChildForcingMove32) {
  BoardData<3, 2> data;
  State state(data);
  using namespace node;
  NodeIndex max_nodes = 10_nind;
  SolutionDag solution(data, max_nodes);
  auto variation = solution.get_variation(bag<Position>{1_pos, 4_pos, 0_pos});
  EXPECT_EQ(1u, solution.children_size(variation));
  auto pos = solution.get_positions(variation);
  EXPECT_EQ(1u, pos.size());
  EXPECT_EQ(2_pos, pos[0]);
  EXPECT_EQ(Turn::O, solution.get_turn(variation));
}

TEST(SolutionDagTest, GetValueOnRoot) {
  BoardData<3, 2> data;
  State state(data);
  using namespace node;
  NodeIndex max_nodes = 10_nind;
  SolutionDag solution(data, max_nodes);
  const node::NodeP root = solution.get_root();
  EXPECT_EQ(BoardValue::UNKNOWN, solution.get_value(root));
}

}
