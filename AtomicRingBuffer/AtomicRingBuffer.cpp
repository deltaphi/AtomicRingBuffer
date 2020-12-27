#include "AtomicRingBuffer/AtomicRingBuffer.h"

namespace AtomicRingBuffer {

AtomicRingBuffer::size_type AtomicRingBuffer::bytesToPointerOrBufferEnd(const size_type lower, const size_type upper,
                                                                        bool isInside) const {
  size_type bytesAvailable = bytesToSectionEnd(lower);
  size_type upperBytesAvailable = bytesToSectionEnd(upper);
  if (upperBytesAvailable < bytesAvailable) {
    bytesAvailable -= upperBytesAvailable;
  } else if (!isInside && upperBytesAvailable == bytesAvailable && !sameSection(lower, upper)) {
    bytesAvailable -= upperBytesAvailable;
  } else if (isInside && lower == upper) {
    bytesAvailable -= upperBytesAvailable;
  }
  return bytesAvailable;
}

AtomicRingBuffer::size_type AtomicRingBuffer::allocate(pointer_type &data, size_type len, bool partial_acceptable) {
  size_type origAllocateIdx = allocateIdx_;
  size_type allocatedBytes = allocate(origAllocateIdx, readIdx_, false, data, len, partial_acceptable);

  size_type newAllocateIdx = origAllocateIdx + allocatedBytes;
  newAllocateIdx = wrapToDoubleBufferIdx(newAllocateIdx);

  if (newAllocateIdx != origAllocateIdx &&
      allocateIdx_.compare_exchange_strong(origAllocateIdx, newAllocateIdx, std::memory_order_acq_rel)) {
    origAllocateIdx = wrapToBufferIdx(origAllocateIdx);
    return allocatedBytes;
  } else {
    return 0;
  }
}

AtomicRingBuffer::size_type AtomicRingBuffer::commit(atomic_size_type &sectionBegin, atomic_size_type &sectionEnd,
                                                     const pointer_type data, size_type len) {
  if (buffer_ <= data && data <= &buffer_[bufferSize_ - 1]) {
    // Check whether there was actually memory allocated that is now being published.
    size_type requestedIndex = (data - buffer_);
    size_type currentWriteIdx = sectionBegin;
    if (requestedIndex != wrapToBufferIdx(currentWriteIdx)) {
      // Out-of-order commit
      return 0;
    }

    size_type numCommitableElems = bytesToPointerOrBufferEnd_inside(currentWriteIdx, sectionEnd);
    if (len > numCommitableElems) {
      len = numCommitableElems;
    }
    size_type newIdx = currentWriteIdx + len;
    newIdx = wrapToDoubleBufferIdx(newIdx);

    // Check if the memory to be published was previously allocated.
    if (sectionBegin.compare_exchange_strong(currentWriteIdx, newIdx, std::memory_order_acq_rel)) {
      return len;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

AtomicRingBuffer::size_type AtomicRingBuffer::allocate(size_type sectionBegin, size_type sectionEnd, bool isInside,
                                                       pointer_type &data, size_type len,
                                                       bool partial_acceptable) const {
  data = nullptr;
  size_type dataAvailable = bytesToPointerOrBufferEnd(sectionBegin, sectionEnd, isInside);

  if (dataAvailable == 0) {
    return 0;
  } else {
    size_type dataStartIdx = wrapToBufferIdx(sectionBegin);
    if (dataAvailable < len) {
      if (partial_acceptable) {
        len = dataAvailable;
      } else {
        return 0;
      }
    }
    data = &buffer_[dataStartIdx];
    return len;
  }
}

}  // namespace AtomicRingBuffer
