#include "tictactoe.hh"
#include "elevator.hh"
#include "gtest/gtest.h"

namespace {

TEST(GeometryTest, CorrectNumberOfLines) {
  EXPECT_EQ(109, (Geometry<5, 3>::line_size));
  EXPECT_EQ(76, (Geometry<4, 3>::line_size));
  EXPECT_EQ(49, (Geometry<3, 3>::line_size));
}

TEST(SymmetryTest, CorrectNumberOfSymmetries) {
  Geometry<5, 3> geom;
  Symmetry sym(geom);
  EXPECT_EQ(192u, sym.symmetries().size());
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
  return {};
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
  EXPECT_EQ(1_mcount, ++elevator[line]);
  EXPECT_EQ(2_mcount, ++elevator[line]);
  EXPECT_EQ(1_mcount, --elevator[line]);
  EXPECT_EQ(0_mcount, --elevator[line]);
}

TEST(ElevatorTest, IterateFloorZero) {
  Elevator<3, 2> elevator;
  vector<Line> expected(8);
  iota(begin(expected), end(expected), 0_line);
  vector<Line> actual;
  for (auto value : elevator.all(0_mcount)) {
    actual.push_back(value);
  }
  sort(begin(actual), end(actual));
  EXPECT_EQ(expected, actual);
}

TEST(ElevatorTest, IterateFloorOne) {
  Elevator<3, 2> elevator;
  ++elevator[5_line];
  ++elevator[2_line];
  vector<Line> expected{2_line, 5_line};
  vector<Line> actual;
  for (auto value : elevator.all(1_mcount)) {
    actual.push_back(value);
  }
  sort(begin(actual), end(actual));
  EXPECT_EQ(expected, actual);
}


TEST(ElevatorTest, IterateFloorTwo) {
  Elevator<3, 2> elevator;
  ++elevator[5_line];
  ++elevator[2_line];
  ++elevator[2_line];
  ++elevator[3_line];
  ++elevator[5_line];
  --elevator[2_line];
  ++elevator[3_line];
  ++elevator[5_line];
  --elevator[5_line];
  vector<Line> expected{3_line, 5_line};
  vector<Line> actual;
  for (auto value : elevator.all(2_mcount)) {
    actual.push_back(value);
  }
  sort(begin(actual), end(actual));
  EXPECT_EQ(expected, actual);
}



}
