#include "minimax.hh"
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

template<typename T>
auto get_parent_checker(T& minimax) {
  return [&](BoardValue child, BoardValue parent, Turn turn, pair<optional<BoardValue>, bool> result) {
    return result == minimax.get_updated_parent_value(child, parent, turn);
  };
}

TEST(MiniMaxTest, GetParentValue) {
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
}

TEST(MiniMaxTest, Check32) {
  BoardData<3, 2> data;
  State state(data);
  auto minimax = MiniMax(state, data);
  auto result = minimax.play(state, Mark::X);
  EXPECT_EQ(BoardValue::DRAW, *result);
  EXPECT_TRUE(minimax.get_solution().validate());
}

TEST(MiniMaxTest, Check33) {
  BoardData<3, 3> data;
  State state(data);
  auto minimax = MiniMax(state, data);
  auto result = minimax.play(state, Mark::X);
  EXPECT_EQ(BoardValue::X_WIN, *result);
  EXPECT_TRUE(minimax.get_solution().validate());
}

bool validate_all_parents(const SolutionTree::Node *node) {
  for (const auto& child_pair : node->children) {
    const auto child_node = child_pair.second.get();
    if (child_node->parent != node) {
      return false;
    }
  }
  return all_of(begin(node->children), end(node->children), [](const auto& child_pair) {
    return validate_all_parents(child_pair.second.get());
  });
}

TEST(SolutionTreeTest, ValidateParents) {
  BoardData<3, 2> data;
  State state(data);
  auto minimax = MiniMax(state, data);
  minimax.play(state, Mark::X);
  auto root = minimax.get_solution().get_root();
  EXPECT_TRUE(validate_all_parents(root));
}

}
