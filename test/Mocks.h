#ifndef __MOCKS_H__
#define __MOCKS_H__

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
  void SetUp() {
    memset(buffer, 0xFF, kBufferSize);
    ringBuffer.init(buffer, kBufferSize);

    EXPECT_EQ(ringBuffer.size(), 0);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }

  // void TearDown() {}
};



#endif  // __MOCKS_H__
