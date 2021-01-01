#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Mocks.h"

#include "AtomicRingBuffer/AtomicRingBuffer.h"

namespace AtomicRingBuffer {

// Test operation of an uninitialized buffer

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer_Size_Capacity) {
  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), 0);
}

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer_Allocate) {
  Mem mem = ringBuffer.allocate(5, false);
  EXPECT_EQ(mem, Mem());

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), 0);
}

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer_Publish) {
  {
    Mem mem{buffer, 5};
    EXPECT_EQ(ringBuffer.publish(mem), 0);
    EXPECT_EQ(mem.ptr, buffer);
  }

  {
    Mem mem;
    mem.len = 5;
    EXPECT_EQ(ringBuffer.publish(mem), 0);
  }
  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), 0);
}

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer_Peek) {
  Mem mem = ringBuffer.peek(5, false);
  EXPECT_EQ(mem, Mem());

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), 0);
}

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer_Consume) {
  Mem mem{buffer, 5};
  EXPECT_EQ(ringBuffer.consume(mem), 0);
  EXPECT_EQ(mem.ptr, buffer);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), 0);

  Mem mem2{nullptr, 5};
  EXPECT_EQ(ringBuffer.consume(mem2), 0);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), 0);
}

// Test operation on an initialized but empty buffer

TEST_F(BufferedAtomicBufferFixture, EmptyBuffer_Size_Capacity) {
  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
}

TEST_F(BufferedAtomicBufferFixture, EmptyBuffer_Allocate) {
  Mem mem = ringBuffer.allocate(5, false);
  EXPECT_EQ(mem.len, 5);
  EXPECT_EQ(mem.ptr, buffer);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
}

TEST_F(BufferedAtomicBufferFixture, EmptyBuffer_Allocate_Oversize) {
  Mem mem = ringBuffer.allocate(15, false);
  EXPECT_EQ(mem, Mem());

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
}

TEST_F(BufferedAtomicBufferFixture, EmptyBuffer_Allocate_Oversize_Partial) {
  Mem mem = ringBuffer.allocate(15, true);
  EXPECT_EQ(mem.len, 10);
  EXPECT_EQ(mem.ptr, buffer);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
}

/*
 * Test publishing without allocating first.
 */
TEST_F(BufferedAtomicBufferFixture, EmptyBuffer_Publish) {
  Mem mem{buffer, 5};
  EXPECT_EQ(ringBuffer.publish(mem), 0);
  EXPECT_EQ(mem.ptr, buffer);

  {
    Mem mem2{nullptr, 5};
    EXPECT_EQ(ringBuffer.publish(mem2), 0);
  }
  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
}

TEST_F(BufferedAtomicBufferFixture, EmptyBuffer_Peek) {
  Mem mem = ringBuffer.peek(5, false);
  EXPECT_EQ(mem.len, 0);
  EXPECT_EQ(mem.ptr, nullptr);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
}

TEST_F(BufferedAtomicBufferFixture, EmptyBuffer_Consume) {
  Mem mem{buffer, 5};
  EXPECT_EQ(ringBuffer.consume(mem), 0);
  EXPECT_EQ(mem.ptr, buffer);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

  {
    Mem mem2{nullptr, 5};
    EXPECT_EQ(ringBuffer.consume(mem2), 0);
  }

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
}

// Test regular operation: Allocate, publish, peek, consume cycle

TEST_F(BufferedAtomicBufferFixture, FullCycle) {
  {
    Mem mem = ringBuffer.allocate(5, false);
    EXPECT_EQ(mem.len, 5);
    EXPECT_EQ(mem.ptr, buffer);
    for (uint8_t i = 0; i < 5; ++i) {
      mem.ptr[i] = i;
    }

    EXPECT_EQ(ringBuffer.size(), 0);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    // TBD Error Cases: Publish without allocating, publish out of sequence data
    EXPECT_EQ(ringBuffer.publish(mem), 5);

    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }

  {
    // TBD Error Cases: Peek partial
    Mem mem = ringBuffer.peek(5, false);
    EXPECT_EQ(mem.len, 5);

    EXPECT_EQ(mem.ptr, buffer);
    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    // Repeated peek
    {
      Mem mem2 = ringBuffer.peek(5, false);
      EXPECT_EQ(mem2.len, 5);
      EXPECT_EQ(mem, mem2);
    }

    for (uint8_t i = 0; i < 5; ++i) {
      EXPECT_EQ(mem.ptr[i], i);
    }
    for (uint8_t i = 5; i < ringBuffer.capacity(); ++i) {
      EXPECT_EQ(buffer[i], 0xFF);
    }

    // TBD Error Cases: Consume without peek, consume out of order
    EXPECT_EQ(ringBuffer.consume(mem), 5);

    EXPECT_EQ(ringBuffer.size(), 0);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }
}

// Test allocate with partial publish and full peek

TEST_F(BufferedAtomicBufferFixture, FullCycle_PartialPublish_FullPeek) {
  {
    Mem mem = ringBuffer.allocate(5, false);
    EXPECT_EQ(mem.len, 5);
    EXPECT_EQ(mem.ptr, buffer);
    for (uint8_t i = 0; i < 3; ++i) {
      mem.ptr[i] = i;
    }

    EXPECT_EQ(ringBuffer.size(), 0);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    mem.len = 3;
    EXPECT_EQ(ringBuffer.publish(mem), 3);

    EXPECT_EQ(ringBuffer.size(), 3);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }

  {
    Mem mem = ringBuffer.peek(5, true);
    EXPECT_EQ(mem.len, 3);

    EXPECT_EQ(mem.ptr, buffer);
    EXPECT_EQ(ringBuffer.size(), 3);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    for (uint8_t i = 0; i < 3; ++i) {
      EXPECT_EQ(mem.ptr[i], i);
    }
    for (uint8_t i = 3; i < ringBuffer.capacity(); ++i) {
      EXPECT_EQ(buffer[i], 0xFF);
    }

    mem.len = 5;
    EXPECT_EQ(ringBuffer.consume(mem), 3);

    EXPECT_EQ(ringBuffer.size(), 0);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }
}

// test allocate, partial publish, allocate, peek for the part
TEST_F(BufferedAtomicBufferFixture, FullCycle_PartialPublish_Allocate_FullPeek) {
  {
    // Allocate 5 bytes but publish only 3 bytes.
    Mem mem = ringBuffer.allocate(5, false);
    EXPECT_EQ(mem.len, 5);
    EXPECT_EQ(mem.ptr, buffer);
    for (uint8_t i = 0; i < 3; ++i) {
      mem.ptr[i] = i;
    }

    EXPECT_EQ(ringBuffer.size(), 0);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    mem.len = 3;
    EXPECT_EQ(ringBuffer.publish(mem), 3);

    EXPECT_EQ(ringBuffer.size(), 3);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }

  {
    // Allocate an additional 5 bytes
    Mem mem = ringBuffer.allocate(5, false);
    EXPECT_EQ(mem.len, 5);
    EXPECT_EQ(mem.ptr, buffer + 5);
    EXPECT_EQ(ringBuffer.size(), 3);
  }

  {
    // Try to read 5 bytes but get only 3
    Mem mem = ringBuffer.peek(5, true);
    EXPECT_EQ(mem.len, 3);

    EXPECT_EQ(mem.ptr, buffer);
    EXPECT_EQ(ringBuffer.size(), 3);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    for (uint8_t i = 0; i < 3; ++i) {
      EXPECT_EQ(mem.ptr[i], i);
    }
    for (uint8_t i = 3; i < ringBuffer.capacity(); ++i) {
      EXPECT_EQ(buffer[i], 0xFF);
    }

    // Try to consume 5 bytes but consume only 3.
    mem.len = 5;
    EXPECT_EQ(ringBuffer.consume(mem), 3);

    EXPECT_EQ(ringBuffer.size(), 0);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }
}

// test repeated consumption
TEST_F(BufferedAtomicBufferFixture, FullCycle_PartialPeek_RepeatedConsume) {
  {
    Mem mem = ringBuffer.allocate(5, false);
    EXPECT_EQ(mem.len, 5);
    EXPECT_EQ(mem.ptr, buffer);
    for (uint8_t i = 0; i < 5; ++i) {
      mem.ptr[i] = i;
    }

    EXPECT_EQ(ringBuffer.publish(mem), 5);

    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }

  {
    Mem mem = ringBuffer.peek(3, false);
    EXPECT_EQ(mem.len, 3);

    EXPECT_EQ(mem.ptr, buffer);
    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    for (uint8_t i = 0; i < 5; ++i) {
      EXPECT_EQ(mem.ptr[i], i);
    }
    for (uint8_t i = 5; i < ringBuffer.capacity(); ++i) {
      EXPECT_EQ(buffer[i], 0xFF);
    }

    // TBD Error Cases: Consume without peek, consume out of order, Consume more than was peeked.
    mem.len = 3;
    EXPECT_EQ(ringBuffer.consume(mem), 3);
    EXPECT_EQ(ringBuffer.size(), 2);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    // Repeat consumption using the same source pointer
    EXPECT_EQ(ringBuffer.consume(mem), 0);
    EXPECT_EQ(ringBuffer.size(), 2);
  }
}

// test partial peek
TEST_F(BufferedAtomicBufferFixture, FullCycle_PartialPeek_RepeatedPeek) {
  {
    Mem mem = ringBuffer.allocate(5, false);
    EXPECT_EQ(mem.len, 5);
    EXPECT_EQ(mem.ptr, buffer);
    for (uint8_t i = 0; i < 5; ++i) {
      mem.ptr[i] = i;
    }

    EXPECT_EQ(ringBuffer.publish(mem), 5);

    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }

  {
    Mem mem = ringBuffer.peek(3, false);
    EXPECT_EQ(mem.len, 3);

    EXPECT_EQ(mem.ptr, buffer);
    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    for (uint8_t i = 0; i < 5; ++i) {
      EXPECT_EQ(mem.ptr[i], i);
    }
    for (uint8_t i = 5; i < ringBuffer.capacity(); ++i) {
      EXPECT_EQ(buffer[i], 0xFF);
    }

    // TBD Error Cases: Consume without peek, consume out of order, Consume more than was peeked.
    mem.len = 3;
    EXPECT_EQ(ringBuffer.consume(mem), 3);
    EXPECT_EQ(ringBuffer.size(), 2);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    // Peek & Consume the remainder
    mem = ringBuffer.peek(2, false);
    EXPECT_EQ(mem.len, 2);

    EXPECT_EQ(mem.ptr, buffer + 3);

    for (uint8_t i = 0; i < 2; ++i) {
      EXPECT_EQ(mem.ptr[i], i + 3);
    }
    EXPECT_EQ(ringBuffer.consume(mem), 2);
    EXPECT_EQ(ringBuffer.size(), 0);
  }
}

// test partial consumption
TEST_F(BufferedAtomicBufferFixture, FullCycle_FullPeek_PartialConsume) {
  {
    // Write 5 bytes to the buffer
    Mem mem = ringBuffer.allocate(5, false);
    EXPECT_EQ(mem.len, 5);
    EXPECT_EQ(mem.ptr, buffer);
    for (uint8_t i = 0; i < 5; ++i) {
      mem.ptr[i] = i;
    }

    EXPECT_EQ(ringBuffer.publish(mem), 5);

    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }

  {
    // Peek 5 bytes
    Mem mem = ringBuffer.peek(5, false);
    EXPECT_EQ(mem.len, 5);

    EXPECT_EQ(mem.ptr, buffer);
    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    for (uint8_t i = 0; i < 5; ++i) {
      EXPECT_EQ(mem.ptr[i], i);
    }
    for (uint8_t i = 5; i < ringBuffer.capacity(); ++i) {
      EXPECT_EQ(buffer[i], 0xFF);
    }

    // Consume 3 bytes
    mem.len = 3;
    EXPECT_EQ(ringBuffer.consume(mem), 3);
    EXPECT_EQ(ringBuffer.size(), 2);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    // Peek & Consume the remainder (2 bytes)

    // Try overpeeking first (won't work)
    mem = ringBuffer.peek(5, false);
    EXPECT_EQ(mem.len, 0);
    EXPECT_EQ(mem.ptr, nullptr);
    // Try overpeeking but accepting a partial result
    mem = ringBuffer.peek(5, true);
    EXPECT_EQ(mem.len, 2);
    EXPECT_EQ(ringBuffer.size(), 2);

    EXPECT_EQ(mem.ptr, buffer + 3);

    for (uint8_t i = 0; i < 2; ++i) {
      EXPECT_EQ(mem.ptr[i], i + 3);
    }
    EXPECT_EQ(ringBuffer.consume(mem), 2);
    EXPECT_EQ(ringBuffer.size(), 0);
  }
}

TEST_F(BufferedAtomicBufferFixture, SkipPublish) {
  {
    Mem mem = ringBuffer.allocate(5, false);
    EXPECT_EQ(mem.len, 5);
    EXPECT_EQ(mem.ptr, buffer);
    for (uint8_t i = 0; i < 5; ++i) {
      mem.ptr[i] = i;
    }

    Mem mem2{mem.ptr + 3, 2};
    EXPECT_EQ(ringBuffer.publish(mem2), 0);

    EXPECT_EQ(ringBuffer.size(), 0);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }
}

}  // namespace AtomicRingBuffer
