#ifndef __MOCKS_H__
#define __MOCKS_H__

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "AtomicRingBuffer/AtomicRingBuffer.h"

namespace AtomicRingBuffer {

class NoBufferAtomicBufferFixture : public ::testing::Test {
 public:
  using Mem = AtomicRingBuffer::MemoryRange;

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
    Mem mem = ringBuffer.allocate(kInitialFill, false);
    ASSERT_EQ(mem.len, kInitialFill);
    ASSERT_NE(mem.ptr, nullptr);

    for (AtomicRingBuffer::size_type i = 0; i < kInitialFill; ++i) {
      mem.ptr[i] = i;
    }

    ASSERT_EQ(ringBuffer.publish(mem), kInitialFill);
    ASSERT_EQ(ringBuffer.size(), kInitialFill);
  }

  // Read some bytes from the start of the buffer
  void consume5BytesAtStart() {
    Mem mem = ringBuffer.peek(5, false);

    EXPECT_EQ(mem.len, 5);
    EXPECT_EQ(mem.ptr, buffer);

    ASSERT_EQ(ringBuffer.consume(mem), 5);
    ASSERT_EQ(ringBuffer.size(), 2);
  }

  constexpr static const AtomicRingBuffer::size_type kInitialFill = 7;
};

}  // namespace AtomicRingBuffer

#endif  // __MOCKS_H__
