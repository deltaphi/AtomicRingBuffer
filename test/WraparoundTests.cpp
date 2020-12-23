#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Mocks.h"

#include "AtomicRingBuffer/AtomicRingBuffer.h"

namespace AtomicRingBuffer {

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
        EXPECT_EQ(mem[j], j + readData) << "Error reading at byte " << static_cast<uint16_t>(readData);
      }
      readData += numBytes;

      ASSERT_EQ(ringBuffer.consume(mem, numBytes), numBytes)
          << "Error consuming at byte " << static_cast<uint16_t>(readData);
    }
  }
}

TEST_F(BufferedAtomicBufferFixture, FullCircle_WriteBeforeRead) {
  {
    AtomicRingBuffer::pointer_type mem = nullptr;

    ASSERT_EQ(ringBuffer.allocate(mem, kBufferSize, false), kBufferSize);
    EXPECT_EQ(ringBuffer.size(), 0);
    // Push so that the buffer is completely full
    ASSERT_EQ(ringBuffer.publish(mem, kBufferSize), kBufferSize);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);

    // Consume the entire buffer
    ASSERT_EQ(ringBuffer.consume(mem, kBufferSize), kBufferSize);
    EXPECT_EQ(ringBuffer.size(), 0);

    // Fill the buffer again
    ASSERT_EQ(ringBuffer.allocate(mem, kBufferSize, false), kBufferSize);
    EXPECT_EQ(ringBuffer.size(), 0);
    ASSERT_EQ(ringBuffer.publish(mem, kBufferSize), kBufferSize);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);
  }

  {
    AtomicRingBuffer::pointer_type mem = nullptr;

    // Consume 3 bytes
    ASSERT_EQ(ringBuffer.peek(mem, 3, true), 3);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);
    ASSERT_NE(mem, nullptr);
    ASSERT_EQ(ringBuffer.consume(mem, 3), 3);
    EXPECT_EQ(ringBuffer.size(), 7);

    mem = nullptr;
    // Consume whatever there is left.
    ASSERT_EQ(ringBuffer.peek(mem, std::numeric_limits<AtomicRingBuffer::size_type>::max(), true), 7);
    EXPECT_EQ(ringBuffer.size(), 7);
    ASSERT_NE(mem, nullptr);
    ASSERT_EQ(ringBuffer.consume(mem, 7), 7);
    EXPECT_EQ(ringBuffer.size(), 0);
  }
}

TEST_F(BufferedAtomicBufferFixture, FullCircle_ReadAtFirstWraparound) {
  {
    AtomicRingBuffer::pointer_type mem = nullptr;

    ASSERT_EQ(ringBuffer.allocate(mem, kBufferSize, false), kBufferSize);
    EXPECT_EQ(ringBuffer.size(), 0);
    // Push so that the buffer is completely full
    ASSERT_EQ(ringBuffer.publish(mem, kBufferSize), kBufferSize);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);

    // Consume 3 bytes
    ASSERT_EQ(ringBuffer.consume(mem, 3), 3);
    EXPECT_EQ(ringBuffer.size(), 7);

    // Fill the buffer again by publishing another 3 bytes
    ASSERT_EQ(ringBuffer.allocate(mem, 3, false), 3);
    EXPECT_EQ(ringBuffer.size(), 7);
    ASSERT_EQ(ringBuffer.publish(mem, 3), 3);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);
  }

  {
    AtomicRingBuffer::pointer_type mem = nullptr;

    // Consume as much as possible and expect 7 bytes
    ASSERT_EQ(ringBuffer.peek(mem, std::numeric_limits<AtomicRingBuffer::size_type>::max(), true), 7);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);
    ASSERT_NE(mem, nullptr);
    ASSERT_EQ(ringBuffer.consume(mem, 7), 7);
    EXPECT_EQ(ringBuffer.size(), 3);

    mem = nullptr;
    // Consume whatever there is left.
    ASSERT_EQ(ringBuffer.peek(mem, std::numeric_limits<AtomicRingBuffer::size_type>::max(), true), 3);
    EXPECT_EQ(ringBuffer.size(), 3);
    ASSERT_NE(mem, nullptr);
    ASSERT_EQ(ringBuffer.consume(mem, 3), 3);
    EXPECT_EQ(ringBuffer.size(), 0);
  }
}

TEST_F(BufferedAtomicBufferFixture, MassiveData) {
  const char* loremIpsum =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Duis at dolor id nisl viverra luctus eget vitae "
      "libero. Donec ultricies, ex ac rhoncus mollis, mi ex tempus nibh, quis rutrum nulla magna id eros. Aliquam erat "
      "volutpat. Nunc in dapibus est, non sagittis arcu. Cras vitae libero non felis tempus posuere nec nec lectus. "
      "Cras quam quam, condimentum ut leo eget, pellentesque venenatis metus. Praesent eget hendrerit enim. Duis "
      "pharetra sapien id turpis facilisis vehicula. Aenean mollis, ex id porta sollicitudin, lorem enim interdum "
      "nisi, quis fringilla mi nisl in nunc. Praesent mauris neque, tempor at justo quis, facilisis sodales est. Nunc "
      "a enim id leo tincidunt varius sit amet in sem. Sed aliquet tristique sem quis sodales. Nullam pellentesque "
      "purus a odio tristique venenatis.";

  EXPECT_EQ(ringBuffer.capacity(), 10);

  AtomicRingBuffer::pointer_type mem = nullptr;
  ASSERT_EQ(ringBuffer.allocate(mem, strlen(loremIpsum), true), 10);
  EXPECT_EQ(ringBuffer.allocate(mem, strlen(loremIpsum), true), 0);

  memcpy(mem, loremIpsum, 10);

  EXPECT_EQ(ringBuffer.publish(mem, 10), 10);
  EXPECT_EQ(ringBuffer.allocate(mem, strlen(loremIpsum), true), 0);
}

}  // namespace AtomicRingBuffer
