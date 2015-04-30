#include "gtest.h"
#include "Utils.h"

TEST(UtilsTest, isMultipleOf) {
  EXPECT_EQ(true,  isMultipleOf<5>(15));
  EXPECT_EQ(false, isMultipleOf<5>(16));
  EXPECT_EQ(true,  isMultipleOf<8>(64));
  EXPECT_EQ(false, isMultipleOf<8>(7));
  EXPECT_EQ(false, isMultipleOf<8>(30));
}

TEST(UtilsTest, modularPlusOne) {
  EXPECT_EQ(1, modularPlusOne<5>(0));
  EXPECT_EQ(1, modularPlusOne<5>(15));
  EXPECT_EQ(0, modularPlusOne<5>(4));
  EXPECT_EQ(4, modularPlusOne<5>(3));
  EXPECT_EQ(1, modularPlusOne<8>(0));
  EXPECT_EQ(1, modularPlusOne<8>(64));
  EXPECT_EQ(0, modularPlusOne<8>(7));
  EXPECT_EQ(5, modularPlusOne<8>(4));
}

TEST(UtilsTest, modularIncrement) {
  unsigned t;
  t = 0;  EXPECT_EQ(1, modularIncrement<5>(t)); EXPECT_EQ(1, t);
  t = 15; EXPECT_EQ(1, modularIncrement<5>(t)); EXPECT_EQ(1, t);
  t = 4;  EXPECT_EQ(0, modularIncrement<5>(t)); EXPECT_EQ(0, t);
  t = 3;  EXPECT_EQ(4, modularIncrement<5>(t)); EXPECT_EQ(4, t);
  t = 0;  EXPECT_EQ(1, modularIncrement<8>(t)); EXPECT_EQ(1, t);
  t = 64; EXPECT_EQ(1, modularIncrement<8>(t)); EXPECT_EQ(1, t);
  t = 7;  EXPECT_EQ(0, modularIncrement<8>(t)); EXPECT_EQ(0, t);
  t = 4;  EXPECT_EQ(5, modularIncrement<8>(t)); EXPECT_EQ(5, t);
}

TEST(UtilsTest, modularDecrement) {
  unsigned t;
  t = 0;  EXPECT_EQ(4, modularDecrement<5>(t)); EXPECT_EQ(4, t);
  t = 15; EXPECT_EQ(4, modularDecrement<5>(t)); EXPECT_EQ(4, t);
  t = 4;  EXPECT_EQ(3, modularDecrement<5>(t)); EXPECT_EQ(3, t);
  t = 1;  EXPECT_EQ(0, modularDecrement<5>(t)); EXPECT_EQ(0, t);
  t = 5;  EXPECT_EQ(4, modularDecrement<5>(t)); EXPECT_EQ(4, t);
  t = 0;  EXPECT_EQ(7, modularDecrement<8>(t)); EXPECT_EQ(7, t);
  t = 64; EXPECT_EQ(7, modularDecrement<8>(t)); EXPECT_EQ(7, t);
  t = 7;  EXPECT_EQ(6, modularDecrement<8>(t)); EXPECT_EQ(6, t);
  t = 4;  EXPECT_EQ(3, modularDecrement<8>(t)); EXPECT_EQ(3, t);
}

