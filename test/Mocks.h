#ifndef __MOCKS_H__
#define __MOCKS_H__

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "AtomicRingBuffer/AtomicRingBuffer.h"

namespace AtomicRingBuffer {

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

class FilledAtomicBufferFixture : public BufferedAtomicBufferFixture {
 public:
  void SetUp() {
    BufferedAtomicBufferFixture::SetUp();
    AtomicRingBuffer::pointer_type mem;
    ASSERT_EQ(ringBuffer.allocate(mem, kInitialFill, false), kInitialFill);
    ASSERT_NE(mem, nullptr);

    for (AtomicRingBuffer::size_type i = 0; i < kInitialFill; ++i) {
      mem[i] = i;
    }

    ASSERT_EQ(ringBuffer.publish(mem, kInitialFill), kInitialFill);
    ASSERT_EQ(ringBuffer.size(), kInitialFill);
  }

  // Read some bytes from the start of the buffer
  void consume5BytesAtStart() {
    AtomicRingBuffer::pointer_type mem = nullptr;

    EXPECT_EQ(ringBuffer.peek(mem, 5, false), 5);
    EXPECT_EQ(mem, buffer);

    ASSERT_EQ(ringBuffer.consume(mem, 5), 5);
    ASSERT_EQ(ringBuffer.size(), 2);
  }

  constexpr static const AtomicRingBuffer::size_type kInitialFill = 7;
};

}  // namespace AtomicRingBuffer

#endif  // __MOCKS_H__
