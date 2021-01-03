#ifndef __MOCKS_H__
#define __MOCKS_H__

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <array>

#include "AtomicRingBuffer/AtomicRingBuffer.h"
#include "AtomicRingBuffer/ObjectRingBuffer.h"

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
      mem.ptr[i] = static_cast<AtomicRingBuffer::value_type>(i);
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

struct MyStruct {
  float f;
  uint16_t i;

  bool operator==(const MyStruct& other) const { return f == other.f && i == other.i; }
};

constexpr static const MyStruct demoElems[] = {MyStruct{3.141592654f, 0xCAFE}, MyStruct{2.71828f, 0xAFFE},
                                               MyStruct{0.9f, 0xFEFE}, MyStruct{42.4344f, 0xABAB}};

template <typename BufferT, typename ElemContainerT>
void publishElements(BufferT& buffer, ElemContainerT elems) {
  for (std::size_t i = 0; i < elems.size(); ++i) {
    auto mem = buffer.allocate(1);
    ASSERT_EQ(mem.len, 1) << "Elem Nr. " << i;
    memcpy(mem.ptr, &elems[i], sizeof(MyStruct));
    EXPECT_EQ(buffer.publish(mem), 1);
  }
}

template <typename BufferT, typename ElemContainerT>
void consumeElements(BufferT& buffer, ElemContainerT elems) {
  for (std::size_t i = 0; i < elems.size(); ++i) {
    auto mem = buffer.peek(1);
    ASSERT_EQ(mem.len, 1);
    EXPECT_EQ(*mem.ptr, elems[i]) << "Elem Nr. " << i;
    EXPECT_EQ(buffer.consume(mem), 1);
  }
}

template <typename BufferT, typename ElemContainerT>
void batchPublish(BufferT& buffer, ElemContainerT elems) {
  auto mem = buffer.allocate(elems.size());
  EXPECT_EQ(mem.len, elems.size());
  ASSERT_NE(mem.ptr, nullptr);
  memcpy(mem.ptr, elems.data(), elems.size() * sizeof(MyStruct));
  EXPECT_EQ(buffer.publish(mem), elems.size());
}

class ObjectRingBufferFixture : public ::testing::Test {
 public:
  void SetUp() { ASSERT_EQ(structBuffer.capacity(), 3); }

  constexpr static const std::size_t kBufferCapacity = 3;

  using StructBuffer_t = ObjectRingBuffer<MyStruct, kBufferCapacity>;

  StructBuffer_t structBuffer;
};

}  // namespace AtomicRingBuffer

#endif  // __MOCKS_H__
