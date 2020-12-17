#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Mocks.h"

#include "AtomicRingBuffer/AtomicRingBuffer.h"

// Test wrap-around publish with/without accept partial

TEST_F(FilledAtomicBufferFixture, MatchingAllocate_Full) {
  AtomicRingBuffer::pointer_type mem = nullptr;
  EXPECT_EQ(ringBuffer.allocate(mem, 3, false), 3);
  EXPECT_EQ(mem, buffer + kInitialFill);
}

TEST_F(FilledAtomicBufferFixture, MatchingAllocate_Partial) {
  AtomicRingBuffer::pointer_type mem = nullptr;
  EXPECT_EQ(ringBuffer.allocate(mem, 3, true), 3);
  EXPECT_EQ(mem, buffer + kInitialFill);
}

TEST_F(FilledAtomicBufferFixture, PartialAllocate_Full) {
  AtomicRingBuffer::pointer_type mem = nullptr;
  EXPECT_EQ(ringBuffer.allocate(mem, 5, false), 0);
  EXPECT_EQ(mem, nullptr);
}

TEST_F(FilledAtomicBufferFixture, PartialAllocate_Partial) {
  AtomicRingBuffer::pointer_type mem = nullptr;
  EXPECT_EQ(ringBuffer.allocate(mem, 5, true), 3);
  EXPECT_EQ(mem, buffer + kInitialFill);
}

TEST_F(FilledAtomicBufferFixture, Consume_MatchingAllocate_Full) {
  consume5BytesAtStart();
  {
    AtomicRingBuffer::pointer_type mem = nullptr;
    EXPECT_EQ(ringBuffer.allocate(mem, 3, true), 3);
    EXPECT_EQ(mem, buffer + kInitialFill);
  }
}

TEST_F(FilledAtomicBufferFixture, Consume_MatchingAllocate_Partial) {
  consume5BytesAtStart();
  {
    AtomicRingBuffer::pointer_type mem = nullptr;
    EXPECT_EQ(ringBuffer.allocate(mem, 3, false), 3);
    EXPECT_EQ(mem, buffer + kInitialFill);
  }
}

TEST_F(FilledAtomicBufferFixture, Consume_PartialAllocate_Full) {
  consume5BytesAtStart();
  {
    AtomicRingBuffer::pointer_type mem = nullptr;
    EXPECT_EQ(ringBuffer.allocate(mem, 5, true), 3);
    EXPECT_EQ(mem, buffer + kInitialFill);
    EXPECT_EQ(ringBuffer.size(), 2);
  }
  {
    AtomicRingBuffer::pointer_type mem = nullptr;
    EXPECT_EQ(ringBuffer.allocate(mem, 5, true), 5);
    EXPECT_EQ(mem, buffer);
    EXPECT_EQ(ringBuffer.size(), 2);
  }
}

TEST_F(FilledAtomicBufferFixture, Consume_PartialAllocate_Partial) {
  consume5BytesAtStart();
  {
    AtomicRingBuffer::pointer_type mem = nullptr;
    EXPECT_EQ(ringBuffer.allocate(mem, 5, false), 0);
    EXPECT_EQ(mem, nullptr);
    EXPECT_EQ(ringBuffer.size(), 2);
  }
  {
    AtomicRingBuffer::pointer_type mem = nullptr;
    EXPECT_EQ(ringBuffer.allocate(mem, 3, false), 3);
    EXPECT_EQ(mem, buffer + kInitialFill);
    EXPECT_EQ(ringBuffer.size(), 2);
  }
  {
    AtomicRingBuffer::pointer_type mem = nullptr;
    EXPECT_EQ(ringBuffer.allocate(mem, 5, false), 5);
    EXPECT_EQ(mem, buffer);
    EXPECT_EQ(ringBuffer.size(), 2);
  }
}

TEST_F(BufferedAtomicBufferFixture, FullCircle_ManyBytes) {
  // Send 200 integers of increasing range in chunks of 3
  uint8_t sendData = 0;
  uint8_t readData = 0;

  while (sendData < 200) {
    // Send two packets and advance sendData by how many bytes were sent.
    for (int i = 0; i < 2; ++i) {
      AtomicRingBuffer::pointer_type mem = nullptr;
      AtomicRingBuffer::size_type numBytes = ringBuffer.allocate(mem, 3, true);
      ASSERT_GT(numBytes, 0);
      ASSERT_LE(numBytes, 3);
      ASSERT_NE(mem, nullptr);

      for (int j = 0; j < numBytes; ++j) {
        mem[j] = sendData + j;
      }
      sendData += numBytes;

      ASSERT_EQ(ringBuffer.publish(mem, numBytes), numBytes)
          << "Error publishing at byte " << static_cast<uint16_t>(sendData);
    }

    // Read integers in chunks of up to 5
    {
      AtomicRingBuffer::pointer_type mem = nullptr;
      AtomicRingBuffer::size_type numBytes = ringBuffer.peek(mem, 5, true);
      ASSERT_GT(numBytes, 0);
      ASSERT_LE(numBytes, 5);
      ASSERT_NE(mem, nullptr);

      for (int j = 0; j < numBytes; ++j) {
        EXPECT_EQ(mem[j], j + readData);
      }
      readData += numBytes;

      ASSERT_EQ(ringBuffer.consume(mem, numBytes), numBytes);
    }
  }
}