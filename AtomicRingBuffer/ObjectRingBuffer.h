#ifndef __ATOMICRINGBUFFER__STRUCTRINGBUFFER_H__
#define __ATOMICRINGBUFFER__STRUCTRINGBUFFER_H__

#include "AtomicRingBuffer.h"

namespace AtomicRingBuffer {

/*
 * \brief Class ObjectRingBuffer
 */
template <typename T, uint16_t elemCapacity>
class ObjectRingBuffer {
 public:
  static_assert(elemCapacity > 0, "Cannot have Buffer with 0 capacity.");

  using Delegate_t = AtomicRingBuffer;
  using size_type = Delegate_t::size_type;
  using value_type = T;
  using pointer_type = value_type*;

 private:
  constexpr static Delegate_t::size_type toNumBytes(const size_type numElems) { return numElems * sizeof(value_type); }

  constexpr static Delegate_t::size_type toNumElements(const Delegate_t::size_type numBytes) {
    return numBytes / sizeof(value_type);
  }

 public:
  constexpr static const size_type kByteBufferSize = ObjectRingBuffer::toNumBytes(elemCapacity);

  using ByteBuffer_t = uint8_t[kByteBufferSize];

  struct MemoryRange {
    pointer_type ptr = nullptr;
    size_type len = 0;

    bool operator==(const MemoryRange& other) const { return ptr == other.ptr && len == other.len; }
  };

  constexpr ObjectRingBuffer() : delegate(bytebuffer, kByteBufferSize) {}

  MemoryRange allocate(const size_type size = 1) {
    return convertMemoryRange(delegate.allocate(toNumBytes(size), false));
  }

  size_type publish(const MemoryRange elemPtr) { return toNumElements(delegate.publish(convertMemoryRange(elemPtr))); }

  const MemoryRange peek(const size_type size = 1) const {
    return convertMemoryRange(delegate.peek(toNumBytes(size), false));
  }

  size_type consume(const MemoryRange elemPtr) { return toNumElements(delegate.consume(convertMemoryRange(elemPtr))); }

  size_type size() const { return toNumElements(delegate.size()); }

  bool empty() const { return delegate.empty(); }

  size_type capacity() const { return toNumElements(delegate.capacity()); }

 private:
  constexpr static MemoryRange convertMemoryRange(const Delegate_t::MemoryRange& delegateRange) {
    const auto len = toNumElements(delegateRange.len);
    return MemoryRange{reinterpret_cast<pointer_type>(delegateRange.ptr), len};
  }

  constexpr static Delegate_t::MemoryRange convertMemoryRange(const MemoryRange& myRange) {
    return Delegate_t::MemoryRange{reinterpret_cast<Delegate_t::pointer_type>(myRange.ptr), toNumBytes(myRange.len)};
  }

  Delegate_t delegate;

  ByteBuffer_t bytebuffer;
};

}  // namespace AtomicRingBuffer

#endif  // __ATOMICRINGBUFFER__STRUCTRINGBUFFER_H__
