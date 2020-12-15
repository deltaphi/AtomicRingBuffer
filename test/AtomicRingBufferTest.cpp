#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "AtomicRingBuffer/AtomicRingBuffer.h"

class NoBufferAtomicBufferFixture : public ::testing::Test {
 public:
  void SetUp() {}

  void TearDown() {}

  AtomicRingBuffer ringBuffer;
};

class BufferedAtomicBufferFixture : public NoBufferAtomicBufferFixture {
 public:
  void SetUp() { ringBuffer.init(buffer, kBufferSize); }

  void TearDown() {}

  constexpr static const AtomicRingBuffer::size_type kBufferSize = 10;
  uint8_t buffer[kBufferSize];
};

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer) {
  uint8_t * mem;
  EXPECT_EQ(ringBuffer.allocate(mem, 5, false), 0);
  EXPECT_EQ(mem, nullptr);
}