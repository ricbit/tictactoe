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


}
