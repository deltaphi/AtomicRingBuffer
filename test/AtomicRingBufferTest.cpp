#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Mocks.h"

#include "AtomicRingBuffer/AtomicRingBuffer.h"


// Test operation of an uninitialized buffer

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer_Size_Capacity) {
  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), 0);
}

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer_Allocate) {
  uint8_t* mem;
  EXPECT_EQ(ringBuffer.allocate(mem, 5, false), 0);
  EXPECT_EQ(mem, nullptr);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), 0);
}

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer_Publish) {
  uint8_t* mem = buffer;
  EXPECT_EQ(ringBuffer.publish(mem, 5), 0);
  EXPECT_EQ(buffer, mem);

  EXPECT_EQ(ringBuffer.publish(nullptr, 5), 0);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), 0);
}

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer_Peek) {
  uint8_t* mem;
  EXPECT_EQ(ringBuffer.peek(mem, 5, false), 0);
  EXPECT_EQ(mem, nullptr);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), 0);
}

TEST_F(NoBufferAtomicBufferFixture, EmptyBuffer_Consume) {
  uint8_t* mem = buffer;
  EXPECT_EQ(ringBuffer.consume(mem, 5), 0);
  EXPECT_EQ(buffer, mem);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), 0);

  EXPECT_EQ(ringBuffer.consume(nullptr, 5), 0);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), 0);
}

// Test operation on an initialized but empty buffer

TEST_F(BufferedAtomicBufferFixture, EmptyBuffer_Size_Capacity) {
  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
}

TEST_F(BufferedAtomicBufferFixture, EmptyBuffer_Allocate) {
  uint8_t* mem;
  EXPECT_EQ(ringBuffer.allocate(mem, 5, false), 5);
  EXPECT_EQ(mem, buffer);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
}

TEST_F(BufferedAtomicBufferFixture, EmptyBuffer_Allocate_Oversize) {
  uint8_t* mem;
  EXPECT_EQ(ringBuffer.allocate(mem, 15, false), 0);
  EXPECT_EQ(mem, nullptr);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
}

TEST_F(BufferedAtomicBufferFixture, EmptyBuffer_Allocate_Oversize_Partial) {
  uint8_t* mem;
  EXPECT_EQ(ringBuffer.allocate(mem, 15, true), 10);
  EXPECT_EQ(mem, buffer);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
}

/*
 * Test publishing without allocating first.
 */
TEST_F(BufferedAtomicBufferFixture, EmptyBuffer_Publish) {
  uint8_t* mem = buffer;
  EXPECT_EQ(ringBuffer.publish(mem, 5), 0);
  EXPECT_EQ(buffer, mem);

  EXPECT_EQ(ringBuffer.publish(nullptr, 5), 0);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
}

TEST_F(BufferedAtomicBufferFixture, EmptyBuffer_Peek) {
  uint8_t* mem;
  EXPECT_EQ(ringBuffer.peek(mem, 5, false), 0);
  EXPECT_EQ(mem, nullptr);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
}

TEST_F(BufferedAtomicBufferFixture, EmptyBuffer_Consume) {
  uint8_t* mem = buffer;
  EXPECT_EQ(ringBuffer.consume(mem, 5), 0);
  EXPECT_EQ(buffer, mem);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

  EXPECT_EQ(ringBuffer.consume(nullptr, 5), 0);

  EXPECT_EQ(ringBuffer.size(), 0);
  EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
}

// Test regular operation: Allocate, publish, peek, consume cycle

TEST_F(BufferedAtomicBufferFixture, FullCycle) {
  {
    uint8_t* mem = nullptr;
    EXPECT_EQ(ringBuffer.allocate(mem, 5, false), 5);
    EXPECT_EQ(buffer, mem);
    for (uint8_t i = 0; i < 5; ++i) {
      mem[i] = i;
    }

    EXPECT_EQ(ringBuffer.size(), 0);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    // TBD Error Cases: Publish without allocating, publish out of sequence data
    EXPECT_EQ(ringBuffer.publish(mem, 5), 5);

    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }

  {
    uint8_t* mem = nullptr;

    // TBD Error Cases: Peek partial
    EXPECT_EQ(ringBuffer.peek(mem, 5, false), 5);

    EXPECT_EQ(buffer, mem);
    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    // Repeated peek
    {
      uint8_t* mem2 = nullptr;
      EXPECT_EQ(ringBuffer.peek(mem2, 5, false), 5);
      EXPECT_EQ(mem, mem2);
    }

    for (uint8_t i = 0; i < 5; ++i) {
      EXPECT_EQ(mem[i], i);
    }
    for (uint8_t i = 5; i < ringBuffer.capacity(); ++i) {
      EXPECT_EQ(buffer[i], 0xFF);
    }

    // TBD Error Cases: Consume without peek, consume out of order
    EXPECT_EQ(ringBuffer.consume(mem, 5), 5);

    EXPECT_EQ(ringBuffer.size(), 0);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }
}

// Test allocate with partial publish and full peek

TEST_F(BufferedAtomicBufferFixture, FullCycle_PartialPublish_FullPeek) {
  {
    uint8_t* mem = nullptr;
    EXPECT_EQ(ringBuffer.allocate(mem, 5, false), 5);
    EXPECT_EQ(buffer, mem);
    for (uint8_t i = 0; i < 3; ++i) {
      mem[i] = i;
    }

    EXPECT_EQ(ringBuffer.size(), 0);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    EXPECT_EQ(ringBuffer.publish(mem, 3), 3);

    EXPECT_EQ(ringBuffer.size(), 3);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }

  {
    uint8_t* mem = nullptr;

    EXPECT_EQ(ringBuffer.peek(mem, 5, false), 3);

    EXPECT_EQ(buffer, mem);
    EXPECT_EQ(ringBuffer.size(), 3);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    for (uint8_t i = 0; i < 3; ++i) {
      EXPECT_EQ(mem[i], i);
    }
    for (uint8_t i = 3; i < ringBuffer.capacity(); ++i) {
      EXPECT_EQ(buffer[i], 0xFF);
    }

    EXPECT_EQ(ringBuffer.consume(mem, 5), 3);

    EXPECT_EQ(ringBuffer.size(), 0);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }
}

// test allocate, partial publish, allocate, peek for the part
TEST_F(BufferedAtomicBufferFixture, FullCycle_PartialPublish_Allocate_FullPeek) {
  {
    uint8_t* mem = nullptr;
    EXPECT_EQ(ringBuffer.allocate(mem, 5, false), 5);
    EXPECT_EQ(buffer, mem);
    for (uint8_t i = 0; i < 3; ++i) {
      mem[i] = i;
    }

    EXPECT_EQ(ringBuffer.size(), 0);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    EXPECT_EQ(ringBuffer.publish(mem, 3), 3);

    EXPECT_EQ(ringBuffer.size(), 3);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }

  {
    uint8_t* mem = nullptr;
    EXPECT_EQ(ringBuffer.allocate(mem, 5, false), 5);
    EXPECT_EQ(mem, buffer + 5);
    EXPECT_EQ(ringBuffer.size(), 3);
  }

  {
    uint8_t* mem = nullptr;

    EXPECT_EQ(ringBuffer.peek(mem, 5, false), 3);

    EXPECT_EQ(buffer, mem);
    EXPECT_EQ(ringBuffer.size(), 3);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    for (uint8_t i = 0; i < 3; ++i) {
      EXPECT_EQ(mem[i], i);
    }
    for (uint8_t i = 3; i < ringBuffer.capacity(); ++i) {
      EXPECT_EQ(buffer[i], 0xFF);
    }

    EXPECT_EQ(ringBuffer.consume(mem, 5), 3);

    EXPECT_EQ(ringBuffer.size(), 0);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }
}

// test repeated consumption
TEST_F(BufferedAtomicBufferFixture, FullCycle_PartialPeek_RepeatedConsume) {
  {
    uint8_t* mem = nullptr;
    EXPECT_EQ(ringBuffer.allocate(mem, 5, false), 5);
    EXPECT_EQ(buffer, mem);
    for (uint8_t i = 0; i < 5; ++i) {
      mem[i] = i;
    }

    EXPECT_EQ(ringBuffer.publish(mem, 5), 5);

    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }

  {
    uint8_t* mem = nullptr;
    EXPECT_EQ(ringBuffer.peek(mem, 3, false), 3);

    EXPECT_EQ(buffer, mem);
    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    for (uint8_t i = 0; i < 5; ++i) {
      EXPECT_EQ(mem[i], i);
    }
    for (uint8_t i = 5; i < ringBuffer.capacity(); ++i) {
      EXPECT_EQ(buffer[i], 0xFF);
    }

    // TBD Error Cases: Consume without peek, consume out of order, Consume more than was peeked.
    EXPECT_EQ(ringBuffer.consume(mem, 3), 3);
    EXPECT_EQ(ringBuffer.size(), 2);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    // Repeat consumption using the same source pointer
    EXPECT_EQ(ringBuffer.consume(mem, 3), 0);
    EXPECT_EQ(ringBuffer.size(), 2);
  }
}

// test partial peek
TEST_F(BufferedAtomicBufferFixture, FullCycle_PartialPeek_RepeatedPeek) {
  {
    uint8_t* mem = nullptr;
    EXPECT_EQ(ringBuffer.allocate(mem, 5, false), 5);
    EXPECT_EQ(buffer, mem);
    for (uint8_t i = 0; i < 5; ++i) {
      mem[i] = i;
    }

    EXPECT_EQ(ringBuffer.publish(mem, 5), 5);

    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }

  {
    uint8_t* mem = nullptr;
    EXPECT_EQ(ringBuffer.peek(mem, 3, false), 3);

    EXPECT_EQ(buffer, mem);
    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    for (uint8_t i = 0; i < 5; ++i) {
      EXPECT_EQ(mem[i], i);
    }
    for (uint8_t i = 5; i < ringBuffer.capacity(); ++i) {
      EXPECT_EQ(buffer[i], 0xFF);
    }

    // TBD Error Cases: Consume without peek, consume out of order, Consume more than was peeked.
    EXPECT_EQ(ringBuffer.consume(mem, 3), 3);
    EXPECT_EQ(ringBuffer.size(), 2);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    // Peek & Consume the remainder
    EXPECT_EQ(ringBuffer.peek(mem, 2, false), 2);

    EXPECT_EQ(buffer + 3, mem);

    for (uint8_t i = 0; i < 2; ++i) {
      EXPECT_EQ(mem[i], i + 3);
    }
    EXPECT_EQ(ringBuffer.consume(mem, 2), 2);
    EXPECT_EQ(ringBuffer.size(), 0);
  }
}

// test partial consumption
TEST_F(BufferedAtomicBufferFixture, FullCycle_FullPeek_PartialConsume) {
  {
    uint8_t* mem = nullptr;
    EXPECT_EQ(ringBuffer.allocate(mem, 5, false), 5);
    EXPECT_EQ(buffer, mem);
    for (uint8_t i = 0; i < 5; ++i) {
      mem[i] = i;
    }

    EXPECT_EQ(ringBuffer.publish(mem, 5), 5);

    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);
  }

  {
    uint8_t* mem = nullptr;
    EXPECT_EQ(ringBuffer.peek(mem, 5, false), 5);

    EXPECT_EQ(buffer, mem);
    EXPECT_EQ(ringBuffer.size(), 5);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    for (uint8_t i = 0; i < 5; ++i) {
      EXPECT_EQ(mem[i], i);
    }
    for (uint8_t i = 5; i < ringBuffer.capacity(); ++i) {
      EXPECT_EQ(buffer[i], 0xFF);
    }

    EXPECT_EQ(ringBuffer.consume(mem, 3), 3);
    EXPECT_EQ(ringBuffer.size(), 2);
    EXPECT_EQ(ringBuffer.capacity(), kBufferSize);

    // Peek & Consume the remainder
    EXPECT_EQ(ringBuffer.peek(mem, 5, false), 2);
    EXPECT_EQ(ringBuffer.size(), 2);

    EXPECT_EQ(buffer + 3, mem);

    for (uint8_t i = 0; i < 2; ++i) {
      EXPECT_EQ(mem[i], i + 3);
    }
    EXPECT_EQ(ringBuffer.consume(mem, 2), 2);
    EXPECT_EQ(ringBuffer.size(), 0);
  }
}

// Test wrap-around publish with/without accept partial