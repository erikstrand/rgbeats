#include "gtest.h"
#include "PJRCAudio/sqrt_integer.h"
#include <iostream>


TEST(sqrt_integer_test, zero_case) {
  EXPECT_EQ(0, sqrt_uint32(0));
  EXPECT_EQ(0, sqrt_uint32_approx(0));
  EXPECT_EQ(0, sqrt_uint64(0));
}

TEST(sqrt_integer_test, known_values_32) {
  EXPECT_EQ(1, sqrt_uint32(1));
  EXPECT_EQ(2, sqrt_uint32(4));
  EXPECT_EQ(3, sqrt_uint32(9));
  EXPECT_EQ(4, sqrt_uint32(16));
  EXPECT_EQ(65535, sqrt_uint32(4294836225));
}

TEST(sqrt_integer_test, known_values_64) {
  EXPECT_EQ(1, sqrt_uint64(1));
  EXPECT_EQ(2, sqrt_uint64(4));
  EXPECT_EQ(3, sqrt_uint64(9));
  EXPECT_EQ(4, sqrt_uint64(16));
  EXPECT_EQ(65535, sqrt_uint64(4294836225));
  EXPECT_EQ(2181845567, sqrt_uint64(4760450081321191490ull));
  EXPECT_EQ(4294967295, sqrt_uint64(18446744065119617025ull));
}

TEST(sqrt_integer_test, convergence) {
  convergence_test(4294836225);
  convergence_test(4760450081321191490ull);
  convergence_test(18446744065119617025ull);
  EXPECT_EQ(1, 1);
}

/*
TEST(sqrt_integer_test, exhaustive) {
  uint32_t root = 1
  uint32_t i = 1;
  for (; i!=0; ++i) {
    if (
  }
}
*/
