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

}
