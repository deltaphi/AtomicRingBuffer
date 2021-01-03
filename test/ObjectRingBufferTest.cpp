#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "AtomicRingBuffer/ObjectRingBuffer.h"
#include "Mocks.h"

namespace AtomicRingBuffer {

TEST_F(ObjectRingBufferFixture, NewBufferIsEmpty) { EXPECT_TRUE(structBuffer.empty()); }

TEST_F(ObjectRingBufferFixture, NewBufferHasSize0) { EXPECT_EQ(structBuffer.size(), 0); }

TEST_F(ObjectRingBufferFixture, AfterOneAllocate_IsEmpty) {
  structBuffer.allocate(1);
  EXPECT_TRUE(structBuffer.empty());
}

TEST_F(ObjectRingBufferFixture, AfterOneAllocate_IsNotNull) {
  EXPECT_GE(structBuffer.capacity(), 1);
  auto mem = structBuffer.allocate(1);
  EXPECT_NE(mem.ptr, nullptr);
  EXPECT_EQ(mem.len, 1);
  EXPECT_TRUE(structBuffer.empty());
}

TEST_F(ObjectRingBufferFixture, AfterOnePublish_IsNotEmpty) {
  auto mem = structBuffer.allocate(1);
  EXPECT_EQ(structBuffer.publish(mem), 1);
  EXPECT_FALSE(structBuffer.empty());
  EXPECT_EQ(structBuffer.size(), 1);
}

TEST_F(ObjectRingBufferFixture, AfterOnePublish_PeekOne) {
  auto mem = structBuffer.allocate(1);
  memcpy(mem.ptr, &demoElems[0], mem.len * sizeof(demoElems[0]));
  EXPECT_EQ(structBuffer.publish(mem), 1);

  auto peek = structBuffer.peek(1);
  EXPECT_EQ(peek.ptr, mem.ptr);
  EXPECT_EQ(peek.len, 1);
  EXPECT_EQ(*peek.ptr, demoElems[0]);
}

TEST_F(ObjectRingBufferFixture, AfterOnePublish_ConsumeOne_IsEmpty) {
  auto mem = structBuffer.allocate(1);
  ASSERT_EQ(mem.len, 1);
  memcpy(mem.ptr, &demoElems[0], sizeof(demoElems[0]));
  EXPECT_EQ(structBuffer.publish(mem), 1);

  EXPECT_EQ(structBuffer.consume(mem), 1);
  EXPECT_EQ(structBuffer.size(), 0);
  EXPECT_TRUE(structBuffer.empty());
}

TEST_F(ObjectRingBufferFixture, AfterTwoPublish_ConsumeOne_CorrectElement_IsNotEmpty) {
  std::array<MyStruct, 2> elems = {demoElems[0], demoElems[1]};

  publishElements(structBuffer, elems);

  auto peek = structBuffer.peek(1);
  EXPECT_EQ(structBuffer.consume(peek), 1);

  EXPECT_EQ(structBuffer.size(), 1);
  EXPECT_FALSE(structBuffer.empty());
}

TEST_F(ObjectRingBufferFixture, AfterTwoPublish_SizeIs2) {
  std::array<MyStruct, 2> elems = {demoElems[0], demoElems[1]};

  publishElements(structBuffer, elems);

  EXPECT_EQ(structBuffer.size(), 2);
  EXPECT_FALSE(structBuffer.empty());
}

TEST_F(ObjectRingBufferFixture, AfterTwoPublish_ConsumeTwo_CorrectElements_IsEmpty) {
  std::array<MyStruct, 2> elems = {demoElems[0], demoElems[1]};

  publishElements(structBuffer, elems);

  consumeElements(structBuffer, elems);

  EXPECT_EQ(structBuffer.size(), 0);
  EXPECT_TRUE(structBuffer.empty());
}

TEST_F(ObjectRingBufferFixture, AfterThreeAllocate_RejectAllocate) {
  {
    auto mem = structBuffer.allocate(3);
    EXPECT_NE(mem.ptr, nullptr);
    EXPECT_EQ(mem.len, 3);
  }

  {
    auto mem = structBuffer.allocate(1);
    EXPECT_EQ(mem, StructBuffer_t::MemoryRange{});
  }
}

TEST_F(ObjectRingBufferFixture, BatchPublishThree) {
  std::array<MyStruct, 3> elems = {demoElems[0], demoElems[1], demoElems[2]};

  batchPublish(structBuffer, elems);

  EXPECT_EQ(structBuffer.size(), 3);
}

TEST_F(ObjectRingBufferFixture, AfterThreePublish_ConsumeOne_PublishOne_ConsumeTwo_ConsumeOne) {
  std::array<MyStruct, 3> elems = {demoElems[0], demoElems[1], demoElems[2]};

  batchPublish(structBuffer, elems);

  {
    std::array<MyStruct, 1> elem = {demoElems[0]};
    consumeElements(structBuffer, elem);
  }
  EXPECT_EQ(structBuffer.size(), 2);

  {
    std::array<MyStruct, 1> elem4 = {demoElems[4]};
    publishElements(structBuffer, elem4);

    std::array<MyStruct, 2> elem2_3 = {demoElems[1], demoElems[2]};
    consumeElements(structBuffer, elem2_3);
    consumeElements(structBuffer, elem4);
  }
}

TEST_F(ObjectRingBufferFixture, AfterTwoPublish_PeekThree_Rejected) {
  std::array<MyStruct, 2> elems = {demoElems[0], demoElems[1]};

  batchPublish(structBuffer, elems);

  auto mem = structBuffer.peek(3);

  EXPECT_EQ(mem.ptr, nullptr);
  EXPECT_EQ(mem.len, 0);
}

}  // namespace AtomicRingBuffer