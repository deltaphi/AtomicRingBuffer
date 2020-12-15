#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "AtomicRingBuffer/AtomicRingBuffer.h"

class NoBufferAtomicBufferFixture : public ::testing::Test {
 public:
  // void SetUp() {}

  // void TearDown() {}

  AtomicRingBuffer ringBuffer;

  constexpr static const AtomicRingBuffer::size_type kBufferSize = 10;
  uint8_t buffer[kBufferSize];
};

class BufferedAtomicBufferFixture : public NoBufferAtomicBufferFixture {
 public:
  void SetUp() { ringBuffer.init(buffer, kBufferSize); }

  // void TearDown() {}
};

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer_Allocate) {
  uint8_t* mem;
  EXPECT_EQ(ringBuffer.allocate(mem, 5, false), 0);
  EXPECT_EQ(mem, nullptr);
}

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer_Publish) {
  uint8_t* mem = buffer;
  EXPECT_EQ(ringBuffer.publish(mem, 5), 0);
  EXPECT_EQ(buffer, mem);

  EXPECT_EQ(ringBuffer.publish(nullptr, 5), 0);
}

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer_Peek) {
  uint8_t* mem;
  EXPECT_EQ(ringBuffer.peek(mem, 5), 0);
  EXPECT_EQ(mem, nullptr);
}

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer_Consume) {
  uint8_t* mem = buffer;
  EXPECT_EQ(ringBuffer.consume(mem, 5), 0);
  EXPECT_EQ(buffer, mem);

  EXPECT_EQ(ringBuffer.consume(nullptr, 5), 0);
}