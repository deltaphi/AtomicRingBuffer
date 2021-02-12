#include "AtomicRingBuffer/AtomicRingBuffer.h"

namespace AtomicRingBuffer {

namespace {
constexpr auto min(const AtomicRingBuffer::size_type left, const AtomicRingBuffer::size_type right)
    -> AtomicRingBuffer::size_type {
  return (left > right) ? right : left;
}
}  // namespace

AtomicRingBuffer::size_type AtomicRingBuffer::bytesToPointerOrBufferEnd(const size_type lower, const size_type upper,
                                                                        bool isInside) const {
  size_type bytesAvailable = bytesRemainingInBuffer(lower);
  size_type upperBytesUsed = bytesRemainingInBuffer(upper);

  // Figure out whether the memory managed by upper eats into the bytesAvailable
  if ((upperBytesUsed < bytesAvailable) ||
      (!isInside && upperBytesUsed == bytesAvailable && !sameSection(lower, upper)) || (isInside && lower == upper)) {
    bytesAvailable -= upperBytesUsed;
  }
  return bytesAvailable;
}

AtomicRingBuffer::MemoryRange AtomicRingBuffer::allocate(size_type numElems, bool partial_acceptable) {
  // Find how many bytes can be allocated
  size_type origAllocateIdx = allocateIdx_;
  MemoryRange allocatedMemory = allocate(origAllocateIdx, readIdx_, false, numElems, partial_acceptable);

  // Make the allocation
  size_type newAllocateIdx = origAllocateIdx + allocatedMemory.len;
  newAllocateIdx = wrapToDoubleBufferIdx(newAllocateIdx);

  if (newAllocateIdx != origAllocateIdx &&
      allocateIdx_.compare_exchange_strong(origAllocateIdx, newAllocateIdx, std::memory_order_acq_rel)) {
  } else {
    allocatedMemory.ptr = nullptr;
    allocatedMemory.len = 0;
  }
  return allocatedMemory;
}

AtomicRingBuffer::MemoryRange AtomicRingBuffer::allocate(const size_type sectionBegin, const size_type sectionEnd,
                                                         const bool isInside, const size_type len,
                                                         const bool partial_acceptable) const {
  MemoryRange memory;
  size_type dataAvailable = bytesToPointerOrBufferEnd(sectionBegin, sectionEnd, isInside);

  if (dataAvailable != 0) {
    size_type dataStartIdx = wrapToBufferIdx(sectionBegin);
    if (dataAvailable >= len || partial_acceptable) {
      memory.len = min(len, dataAvailable);
      memory.ptr = &buffer_[dataStartIdx];
    }
  }
  return memory;
}

AtomicRingBuffer::size_type AtomicRingBuffer::commit(atomic_size_type &sectionBegin, atomic_size_type &sectionEnd,
                                                     const MemoryRange data) {
  if (buffer_ <= data.ptr && data.ptr <= &buffer_[bufferSize_ - 1]) {
    // Check whether there was actually memory allocated that is now being published.
    const size_type requestedIndex = (data.ptr - buffer_);
    size_type currentWriteIdx = sectionBegin;
    if (requestedIndex != wrapToBufferIdx(currentWriteIdx)) {
      // Reject out-of-order commit
      return 0;
    }

    const size_type numCommitableElems = bytesToPointerOrBufferEnd_inside(currentWriteIdx, sectionEnd);
    const size_type commitedLen{min(data.len, numCommitableElems)};
    const size_type newIdx = wrapToDoubleBufferIdx(currentWriteIdx + commitedLen);

    // Check if the memory to be published was previously allocated.
    if (sectionBegin.compare_exchange_strong(currentWriteIdx, newIdx, std::memory_order_acq_rel)) {
      return commitedLen;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

}  // namespace AtomicRingBuffer
