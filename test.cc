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
  State cloned(data);
  auto pos = get_first_open_position(cloned);
  EXPECT_TRUE(pos.has_value());
  cloned.play(*pos, Mark::X);
  EXPECT_NE(
      cloned.get_open_positions(Mark::X).count(),
      original.get_open_positions(Mark::X).count());
}

}
