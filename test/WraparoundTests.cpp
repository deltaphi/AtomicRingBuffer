#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Mocks.h"

#include "AtomicRingBuffer/AtomicRingBuffer.h"

namespace AtomicRingBuffer {

// Test wrap-around publish with/without accept partial

TEST_F(FilledAtomicBufferFixture, MatchingAllocate_Full) {
  Mem mem = ringBuffer.allocate(3, false);
  EXPECT_EQ(mem.len, 3);
  EXPECT_EQ(mem.ptr, buffer + kInitialFill);
}

TEST_F(FilledAtomicBufferFixture, MatchingAllocate_Partial) {
  Mem mem = ringBuffer.allocate(3, true);
  EXPECT_EQ(mem.len, 3);
  EXPECT_EQ(mem.ptr, buffer + kInitialFill);
}

TEST_F(FilledAtomicBufferFixture, PartialAllocate_Full) {
  Mem mem = ringBuffer.allocate(5, false);
  EXPECT_EQ(mem, Mem());
}

TEST_F(FilledAtomicBufferFixture, PartialAllocate_Partial) {
  Mem mem = ringBuffer.allocate(5, true);
  EXPECT_EQ(mem.len, 3);
  EXPECT_EQ(mem.ptr, buffer + kInitialFill);
}

TEST_F(FilledAtomicBufferFixture, Consume_MatchingAllocate_Full) {
  consume5BytesAtStart();
  {
    Mem mem = ringBuffer.allocate(3, true);
    EXPECT_EQ(mem.len, 3);
    EXPECT_EQ(mem.ptr, buffer + kInitialFill);
  }
}

TEST_F(FilledAtomicBufferFixture, Consume_MatchingAllocate_Partial) {
  consume5BytesAtStart();
  {
    Mem mem = ringBuffer.allocate(3, false);
    EXPECT_EQ(mem.len, 3);
    EXPECT_EQ(mem.ptr, buffer + kInitialFill);
  }
}

TEST_F(FilledAtomicBufferFixture, Consume_PartialAllocate_Full) {
  consume5BytesAtStart();
  {
    Mem mem = ringBuffer.allocate(5, true);
    EXPECT_EQ(mem.len, 3);
    EXPECT_EQ(mem.ptr, buffer + kInitialFill);
    EXPECT_EQ(ringBuffer.size(), 2);
  }
  {
    Mem mem = ringBuffer.allocate(5, true);
    EXPECT_EQ(mem.len, 5);
    EXPECT_EQ(mem.ptr, buffer);
    EXPECT_EQ(ringBuffer.size(), 2);
  }
}

TEST_F(FilledAtomicBufferFixture, Consume_PartialAllocate_Partial) {
  consume5BytesAtStart();
  {
    Mem mem = ringBuffer.allocate(5, false);
    EXPECT_EQ(mem.len, 0);
    EXPECT_EQ(mem.ptr, nullptr);
    EXPECT_EQ(ringBuffer.size(), 2);
  }
  {
    Mem mem = ringBuffer.allocate(3, false);
    EXPECT_EQ(mem.len, 3);
    EXPECT_EQ(mem.ptr, buffer + kInitialFill);
    EXPECT_EQ(ringBuffer.size(), 2);
  }
  {
    Mem mem = ringBuffer.allocate(5, false);
    EXPECT_EQ(mem.len, 5);
    EXPECT_EQ(mem.ptr, buffer);
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
      Mem mem = ringBuffer.allocate(3, true);
      ASSERT_GT(mem.len, 0);
      ASSERT_LE(mem.len, 3);
      ASSERT_NE(mem.ptr, nullptr);
      EXPECT_EQ(ringBuffer.size(), sendData - readData);

      for (AtomicRingBuffer::size_type j = 0; j < mem.len; ++j) {
        mem.ptr[j] = static_cast<AtomicRingBuffer::value_type>(sendData + j);
      }
      sendData += static_cast<AtomicRingBuffer::value_type>(mem.len);

      ASSERT_EQ(ringBuffer.publish(mem), mem.len) << "Error publishing at byte " << static_cast<uint16_t>(sendData);
      EXPECT_EQ(ringBuffer.size(), sendData - readData);
    }

    // Read integers in chunks of up to 5
    {
      EXPECT_EQ(ringBuffer.size(), sendData - readData);
      Mem mem = ringBuffer.peek(5, true);
      ASSERT_GT(mem.len, 0);
      ASSERT_LE(mem.len, 5);
      ASSERT_NE(mem.ptr, nullptr);

      for (AtomicRingBuffer::size_type j = 0; j < mem.len; ++j) {
        EXPECT_EQ(mem.ptr[j], j + readData) << "Error reading at byte " << static_cast<uint16_t>(readData);
      }
      readData += static_cast<AtomicRingBuffer::value_type>(mem.len);

      ASSERT_EQ(ringBuffer.consume(mem), mem.len) << "Error consuming at byte " << static_cast<uint16_t>(readData);
      EXPECT_EQ(ringBuffer.size(), sendData - readData);
    }
  }
}

TEST_F(BufferedAtomicBufferFixture, FullCircle_WriteBeforeRead) {
  {
    Mem mem = ringBuffer.allocate(kBufferSize, false);
    ASSERT_EQ(mem.len, kBufferSize);
    EXPECT_EQ(ringBuffer.size(), 0);
    // Push so that the buffer is completely full
    ASSERT_EQ(ringBuffer.publish(mem), kBufferSize);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);

    // Consume the entire buffer
    ASSERT_EQ(ringBuffer.consume(mem), kBufferSize);
    EXPECT_EQ(ringBuffer.size(), 0);

    // Fill the buffer again
    mem = ringBuffer.allocate(kBufferSize, false);
    ASSERT_EQ(mem.len, kBufferSize);
    EXPECT_EQ(ringBuffer.size(), 0);
    ASSERT_EQ(ringBuffer.publish(mem), kBufferSize);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);
  }

  {
    // Consume 3 bytes
    Mem mem = ringBuffer.peek(3, true);
    ASSERT_EQ(mem.len, 3);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);
    ASSERT_NE(mem.ptr, nullptr);
    ASSERT_EQ(ringBuffer.consume(mem), 3);
    EXPECT_EQ(ringBuffer.size(), 7);

    // Consume whatever there is left.
    mem = ringBuffer.peek(std::numeric_limits<AtomicRingBuffer::size_type>::max(), true);
    ASSERT_EQ(mem.len, 7);
    EXPECT_EQ(ringBuffer.size(), 7);
    ASSERT_NE(mem.ptr, nullptr);
    ASSERT_EQ(ringBuffer.consume(mem), 7);
    EXPECT_EQ(ringBuffer.size(), 0);
  }
}

TEST_F(BufferedAtomicBufferFixture, FullCircle_LessOneByte) {
  {
    Mem mem = ringBuffer.allocate(kBufferSize, false);
    ASSERT_EQ(mem.len, kBufferSize);
    EXPECT_EQ(ringBuffer.size(), 0);
    // Push so that the buffer is completely full
    ASSERT_EQ(ringBuffer.publish(mem), kBufferSize);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);

    // Consume the entire buffer less one byte
    mem.len = kBufferSize - 1;
    ASSERT_EQ(ringBuffer.consume(mem), kBufferSize - 1);
    EXPECT_EQ(ringBuffer.size(), 1);

    // Fill the buffer again
    mem = ringBuffer.allocate(kBufferSize, true);
    ASSERT_EQ(mem.len, kBufferSize - 1);
    EXPECT_EQ(ringBuffer.size(), 1);
    ASSERT_EQ(ringBuffer.publish(mem), kBufferSize - 1);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);
  }

  {
    // Consume 1 byte before wraparound
    Mem mem = ringBuffer.peek(3, true);
    ASSERT_EQ(mem.len, 1);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);
    ASSERT_NE(mem.ptr, nullptr);
    ASSERT_EQ(ringBuffer.consume(mem), 1);
    EXPECT_EQ(ringBuffer.size(), kBufferSize - 1);

    mem = ringBuffer.peek(std::numeric_limits<AtomicRingBuffer::size_type>::max(), true);
    // Consume whatever there is left.
    ASSERT_EQ(mem.len, 9);
    EXPECT_EQ(ringBuffer.size(), 9);
    ASSERT_NE(mem.ptr, nullptr);
    ASSERT_EQ(ringBuffer.consume(mem), 9);
    EXPECT_EQ(ringBuffer.size(), 0);
  }
}

TEST_F(BufferedAtomicBufferFixture, FullCircle_ReadAtFirstWraparound) {
  {
    Mem mem = ringBuffer.allocate(kBufferSize, false);
    ASSERT_EQ(mem.len, kBufferSize);
    EXPECT_EQ(ringBuffer.size(), 0);
    // Push so that the buffer is completely full
    ASSERT_EQ(ringBuffer.publish(mem), kBufferSize);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);

    // Consume 3 bytes
    mem.len = 3;
    ASSERT_EQ(ringBuffer.consume(mem), 3);
    EXPECT_EQ(ringBuffer.size(), 7);

    // Fill the buffer again by publishing another 3 bytes
    mem = ringBuffer.allocate(3, false);
    ASSERT_EQ(mem.len, 3);
    EXPECT_EQ(ringBuffer.size(), 7);
    ASSERT_EQ(ringBuffer.publish(mem), 3);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);
  }

  {
    // Consume as much as possible and expect 7 bytes
    Mem mem = ringBuffer.peek(std::numeric_limits<AtomicRingBuffer::size_type>::max(), true);
    ASSERT_EQ(mem.len, 7);
    EXPECT_EQ(ringBuffer.size(), kBufferSize);
    ASSERT_NE(mem.ptr, nullptr);
    ASSERT_EQ(ringBuffer.consume(mem), 7);
    EXPECT_EQ(ringBuffer.size(), 3);

    mem = ringBuffer.peek(std::numeric_limits<AtomicRingBuffer::size_type>::max(), true);
    // Consume whatever there is left.
    ASSERT_EQ(mem.len, 3);
    EXPECT_EQ(ringBuffer.size(), 3);
    ASSERT_NE(mem.ptr, nullptr);
    ASSERT_EQ(ringBuffer.consume(mem), 3);
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

  Mem mem = ringBuffer.allocate(strlen(loremIpsum), true);
  ASSERT_EQ(mem.len, 10);

  {
    Mem mem2 = ringBuffer.allocate(strlen(loremIpsum), true);
    EXPECT_EQ(mem2.len, 0);
  }

  memcpy(mem.ptr, loremIpsum, 10);

  EXPECT_EQ(ringBuffer.publish(mem), 10);
  mem = ringBuffer.allocate(strlen(loremIpsum), true);
  EXPECT_EQ(mem.len, 0);
  EXPECT_EQ(ringBuffer.size(), 10);
}

}  // namespace AtomicRingBuffer
