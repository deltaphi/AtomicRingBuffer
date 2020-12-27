#include "AtomicRingBuffer/AtomicRingBuffer.h"

namespace AtomicRingBuffer {

AtomicRingBuffer::size_type AtomicRingBuffer::bytesTillPointerOrBufferEnd(const size_type lower, const size_type upper,
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

AtomicRingBuffer::size_type AtomicRingBuffer::allocate(pointer_type &memory, size_type numElems,
                                                       bool partial_acceptable) {
  memory = nullptr;

  size_type origAllocateIdx = allocateIdx_;

  size_type availableBytes = bytesTillPointerOrBufferEnd_outside(origAllocateIdx, readIdx_);

  if (numElems > availableBytes) {
    // Not as much space available as requested
    if (partial_acceptable) {
      numElems = availableBytes;
    } else {
      return 0;
    }
  }

  size_type newAllocateIdx = origAllocateIdx + numElems;
  newAllocateIdx = wrapToDoubleBufferIdx(newAllocateIdx);

  if (newAllocateIdx != origAllocateIdx &&
      allocateIdx_.compare_exchange_strong(origAllocateIdx, newAllocateIdx, std::memory_order_acq_rel)) {
    origAllocateIdx = wrapToBufferIdx(origAllocateIdx);
    memory = &buffer_[origAllocateIdx];
    return numElems;
  } else {
    return 0;
  }
}

AtomicRingBuffer::size_type AtomicRingBuffer::publish(const pointer_type data, size_type numElems) {
  if (buffer_ <= data && data <= &buffer_[bufferSize_ - 1]) {
    // Check whether there was actually memory allocated that is now being published.
    size_type requestedIndex = (data - buffer_);
    size_type currentWriteIdx = writeIdx_;
    if (requestedIndex != wrapToBufferIdx(currentWriteIdx)) {
      // Out-of-order commit
      return 0;
    }

    size_type numElemsPublishable = bytesTillPointerOrBufferEnd_inside(currentWriteIdx, allocateIdx_);
    if (numElems > numElemsPublishable) {
      numElems = numElemsPublishable;
    }
    size_type newIdx = currentWriteIdx + numElems;
    newIdx = wrapToDoubleBufferIdx(newIdx);

    // Check if the memory to be published was previously allocated.
    if (writeIdx_.compare_exchange_strong(currentWriteIdx, newIdx, std::memory_order_acq_rel)) {
      return numElems;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

AtomicRingBuffer::size_type AtomicRingBuffer::peek(pointer_type &data, size_type len, bool partial_acceptable) const {
  data = nullptr;
  size_type readIdx = readIdx_;
  size_type dataAvailable = bytesTillPointerOrBufferEnd_inside(readIdx, writeIdx_);

  if (dataAvailable == 0) {
    return 0;
  } else {
    readIdx = wrapToBufferIdx(readIdx);
    if (dataAvailable < len) {
      if (partial_acceptable) {
        len = dataAvailable;
      } else {
        return 0;
      }
    }
    data = &buffer_[readIdx];
    return len;
  }
}

AtomicRingBuffer::size_type AtomicRingBuffer::consume(const pointer_type data, size_type len) {
  if (buffer_ <= data && data <= &buffer_[bufferSize_ - 1]) {
    size_type requestedIdx = data - buffer_;
    size_type currentReadIdx = readIdx_;
    if (requestedIdx != wrapToBufferIdx(currentReadIdx)) {
      // Check for out-of-order consume
      return 0;
    }
    size_type numElemsFreeable = bytesTillPointerOrBufferEnd_inside(currentReadIdx, writeIdx_);
    if (len > numElemsFreeable) {
      len = numElemsFreeable;
    }
    size_type newReadIdx = currentReadIdx + len;
    newReadIdx = wrapToDoubleBufferIdx(newReadIdx);
    if (readIdx_.compare_exchange_strong(currentReadIdx, newReadIdx, std::memory_order_acq_rel)) {
      return len;
    } else {
      return 0;
    }

  } else {
    return 0;
  }
}

}  // namespace AtomicRingBuffer
