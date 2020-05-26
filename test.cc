#include "tictactoe.hh"
#include "gtest/gtest.h"

namespace {

TEST(GeometryTest, CorrectNumberOfLines) {
  EXPECT_EQ(109, (Geometry<5, 3>::line_size));
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
  int i = 0;
  for (auto it = begin(tracking); it != end(tracking); ++it) {
    EXPECT_EQ(expected[i++], *it);
  }
}

TEST(TrackingListTest, DeleteElements) {
  TrackingList<5, 1> tracking;
  array expected{1_pos, 3_pos};
  tracking.remove(0_pos);
  tracking.remove(2_pos);
  tracking.remove(4_pos);
  int i = 0;
  for (auto it = begin(tracking); it != end(tracking); ++it) {
    EXPECT_EQ(expected[i++], *it);
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

}
