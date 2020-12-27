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

  if (availableBytes == 0) {
    return 0;
  }

  if (numElems > availableBytes) {
    // Not enough space available
    if (partial_acceptable) {
      numElems = availableBytes;
    } else {
      return 0;
    }
  }

  size_type newAllocateIdx = origAllocateIdx + numElems;
  if (newAllocateIdx >= (2 * bufferSize_)) {
    newAllocateIdx -= 2 * bufferSize_;
  }

  if (allocateIdx_.compare_exchange_strong(origAllocateIdx, newAllocateIdx, std::memory_order_acq_rel)) {
    if (origAllocateIdx >= bufferSize_) {
      origAllocateIdx -= bufferSize_;
    }
    memory = &buffer_[origAllocateIdx];
    return numElems;
  } else {
    return 0;
  }
}

AtomicRingBuffer::size_type AtomicRingBuffer::publish(const pointer_type allocationStart, size_type numElems) {
  if (buffer_ <= allocationStart && allocationStart <= &buffer_[bufferSize_ - 1]) {
    // Check whether there was actually memory allocated that is now being published.
    size_type oldIdx = writeIdx_;
    if (oldIdx != allocateIdx_) {  // TODO: Check is problematic. This will fail if the entire buffer is allocated!
      // options to solve: Leave guard bytes between write -> allocate -> read
      // other options: Extend *Idx_ with a bit that states whether this already wrapped around. Requires fiddly
      // management to not doubly-allocate memory!

      size_type requestedIndex = (allocationStart - buffer_);

      if (oldIdx >= bufferSize_) {
        // Instead of wrapping oldIdx down, wrap requestedIndex up.
        // Avoids having to readjust oldIdx fpr the compare_exchange_strong further below.
        requestedIndex += bufferSize_;
      }

      if (oldIdx != requestedIndex) {
        // Out-of-order commit
        return 0;
      } else {
        size_type newIdx = oldIdx + numElems;
        if (newIdx > (2 * bufferSize_)) {
          // Something went wrong, don't modify anything
          return 0;
        } else if (newIdx == (2 * bufferSize_)) {
          // Wrap around
          newIdx = 0;
        }

        // Check if the memory to be published was previously allocated.
        if (writeIdx_.compare_exchange_strong(oldIdx, newIdx, std::memory_order_acq_rel)) {
          return numElems;
        } else {
          return 0;
        }
      }
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
    if (readIdx >= bufferSize_) {
      readIdx -= bufferSize_;
    }
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
    if (currentReadIdx >= bufferSize_) {
      requestedIdx += bufferSize_;
    }
    if (requestedIdx != currentReadIdx) {
      return 0;
    }
    // not past-the-end
    size_type currentWriteIdx = writeIdx_;
    size_type endOfReadableArea = (currentReadIdx <= currentWriteIdx) ? currentWriteIdx : (2 * bufferSize_);
    size_type numElemsFreeable = endOfReadableArea - currentReadIdx;
    if (numElemsFreeable > bufferSize_) {
      numElemsFreeable -= bufferSize_;
    }
    if (numElemsFreeable == 0) {
      return 0;
    }
    if (len > numElemsFreeable) {
      len = numElemsFreeable;
    }
    size_type newReadIdx = currentReadIdx + len;
    if (newReadIdx > 2 * bufferSize_) {
      // Something went wrong.
      return 0;
    } else if (newReadIdx == (2 * bufferSize_)) {
      newReadIdx = 0;
    }
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
