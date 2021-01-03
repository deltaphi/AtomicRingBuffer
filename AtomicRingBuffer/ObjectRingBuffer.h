#ifndef __ATOMICRINGBUFFER__STRUCTRINGBUFFER_H__
#define __ATOMICRINGBUFFER__STRUCTRINGBUFFER_H__

#include <type_traits>

#include "AtomicRingBuffer.h"

namespace AtomicRingBuffer {

/*
 * \brief Class ObjectRingBuffer
 */
template <typename T, uint16_t elemCapacity, size_t alignment = alignof(T)>
class ObjectRingBuffer {
 public:
  static_assert(elemCapacity > 0, "Cannot have Buffer with 0 capacity.");

  using Delegate_t = AtomicRingBuffer;
  using size_type = Delegate_t::size_type;
  using value_type = T;
  using pointer_type = value_type*;

 private:
  using buffer_element_type = typename std::aligned_storage<sizeof(value_type), alignment>::type;

  constexpr static Delegate_t::size_type toNumBytes(const size_type numElems) {
    return numElems * sizeof(buffer_element_type);
  }

  constexpr static Delegate_t::size_type toNumElements(const Delegate_t::size_type numBytes) {
    return numBytes / sizeof(buffer_element_type);
  }

 public:
  struct MemoryRange {
    pointer_type ptr = nullptr;
    size_type len = 0;

    bool operator==(const MemoryRange& other) const { return ptr == other.ptr && len == other.len; }
  };

  constexpr ObjectRingBuffer() : delegate(bytebuffer, kByteBufferSize) {}

  MemoryRange allocate() { return convertMemoryRange(delegate.allocate(toNumBytes(1), false)); }

  size_type publish(const MemoryRange& elemPtr) { return toNumElements(delegate.publish(convertMemoryRange(elemPtr))); }

  const MemoryRange peek() const { return convertMemoryRange(delegate.peek(toNumBytes(1), false)); }

  size_type consume(const MemoryRange elemPtr) { return toNumElements(delegate.consume(convertMemoryRange(elemPtr))); }

  size_type size() const { return toNumElements(delegate.size()); }

  bool empty() const { return delegate.empty(); }

  size_type capacity() const { return toNumElements(delegate.capacity()); }

 private:
  constexpr static const size_type kByteBufferSize = ObjectRingBuffer::toNumBytes(elemCapacity);

  using ByteBuffer_t = alignas(buffer_element_type) uint8_t[kByteBufferSize];

  constexpr static MemoryRange convertMemoryRange(const Delegate_t::MemoryRange& delegateRange) {
    const auto len = toNumElements(delegateRange.len);
    return MemoryRange{reinterpret_cast<pointer_type>(delegateRange.ptr), len};
  }

  constexpr static Delegate_t::MemoryRange convertMemoryRange(const MemoryRange& myRange) {
    return Delegate_t::MemoryRange{reinterpret_cast<Delegate_t::pointer_type>(myRange.ptr), toNumBytes(myRange.len)};
  }

  ByteBuffer_t bytebuffer;

  Delegate_t delegate;
};

}  // namespace AtomicRingBuffer

#endif  // __ATOMICRINGBUFFER__STRUCTRINGBUFFER_H__
