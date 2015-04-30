#include "gtest.h"
#include "RingBuffer.h"

class RingBufferTest : public ::testing::Test {
protected:
  const static int N = 4;
  RingBuffer<int, N> rb;
  RingBuffer<int, N> rb2;

protected:
  virtual void SetUp () {
    rb2.addSample(1);
    rb2.addSample(2);
    rb2.addSample(3);
    rb2.addSample(4);
    rb2.addSample(5);
    rb2.addSample(6);
  }
};

class RingBufferWithMedianTest : public ::testing::Test {
protected:
  const static int N = 4;
  RingBufferWithMedian<N> empty;
  RingBufferWithMedian<32> rbwm;
  RingBufferWithMedian<32> max;

protected:
  virtual void SetUp () {
    rbwm.addSample(983); 
    rbwm.addSample(984); 
    rbwm.addSample(985); 
    rbwm.addSample(987); 
    rbwm.addSample(987); 
    rbwm.addSample(987); 
    rbwm.addSample(989); 
    rbwm.addSample(989); 
    rbwm.addSample(989); 
    rbwm.addSample(990); 
    rbwm.addSample(990); 
    rbwm.addSample(990); 
    rbwm.addSample(990); 
    rbwm.addSample(991); 
    rbwm.addSample(992); 
    rbwm.addSample(995); 
    rbwm.addSample(997); 
    rbwm.addSample(1001); 
    rbwm.addSample(1001); 
    rbwm.addSample(1003); 
    rbwm.addSample(1003); 
    rbwm.addSample(1005); 
    rbwm.addSample(1006); 
    rbwm.addSample(1007); 
    rbwm.addSample(1011); 
    rbwm.addSample(1012); 
    rbwm.addSample(1012); 
    rbwm.addSample(1012); 
    rbwm.addSample(1025); 
    rbwm.addSample(1026); 
    rbwm.addSample(1029); 
    rbwm.addSample(1032); 

    for (int i=0; i<16; ++i) { max.addSample(-2147483648); }
    for (int i=0; i<16; ++i) { max.addSample( 2147483647); }
  }
};

TEST_F(RingBufferTest, initial_state) {
  for (int i=0; i<N; ++i) {
    EXPECT_EQ(0, rb.nthNewestSample(i));
  }
}

TEST_F(RingBufferTest, bookkeeping) {
  EXPECT_EQ(6, rb2.samples());
  EXPECT_EQ(3, rb2.oldestSample());
  EXPECT_EQ(6, rb2.newestSample());
  // note that this function is zero indexed
  EXPECT_EQ(5, rb2.nthNewestSample(1));
}

TEST_F(RingBufferWithMedianTest, initial_state) {
  for (int i=0; i<N; ++i) {
    EXPECT_EQ(0, empty.nthNewestSample(i));
    EXPECT_EQ(0, empty.nthSmallestSample(i));
  }
}

TEST_F(RingBufferWithMedianTest, bookkeeping) {
  EXPECT_EQ(32, rbwm.samples());
  EXPECT_EQ(983, rbwm.smallestSample());
  EXPECT_EQ(1032, rbwm.largestSample());
  // note that this function is zero indexed
  EXPECT_EQ(991, rbwm.nthSmallestSample(13));
}

TEST_F(RingBufferWithMedianTest, statistics_known_values) {
  EXPECT_EQ(32000, rbwm.sum());
  EXPECT_EQ(1000, rbwm.mean());
  EXPECT_EQ(14, rbwm.stddeviation());
}

TEST_F(RingBufferWithMedianTest, statistics_zeros) {
  EXPECT_EQ(0, empty.sum());
  EXPECT_EQ(0, empty.mean());
  EXPECT_EQ(0, empty.stddeviation());
}

TEST_F(RingBufferWithMedianTest, statistics_overflow) {
  EXPECT_EQ(-16, max.sum());
  EXPECT_EQ(0, max.mean());
  EXPECT_EQ(2181845567, max.stddeviation());
}

