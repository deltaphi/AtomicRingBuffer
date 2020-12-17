#include "AtomicRingBuffer/AtomicRingBuffer.h"

AtomicRingBuffer::size_type AtomicRingBuffer::allocate(pointer_type &memory, size_type numElems,
                                                       bool partial_acceptable) {
  memory = nullptr;

  // Load expected start of writable area
  size_type currentAllocateIdx = allocateIdx_.load();

  // Find out whether end of buffer area or readIdx is ahead
  size_type availableBytes;
  if (readIdx_ <= currentAllocateIdx) {
    availableBytes = bufferSize_ - currentAllocateIdx;
  } else {
    availableBytes = readIdx_ - currentAllocateIdx;
  }

  if (numElems > availableBytes) {
    // Not enough space available
    if (partial_acceptable) {
      numElems = availableBytes;
    } else {
      return 0;
    }
  }

  size_type newAllocateIdx = currentAllocateIdx + numElems;

  if (allocateIdx_.compare_exchange_strong(currentAllocateIdx, newAllocateIdx)) {
    memory = &buffer_[currentAllocateIdx];
    return numElems;
  } else {
    return 0;
  }
}

AtomicRingBuffer::size_type AtomicRingBuffer::publish(const pointer_type allocationStart, size_type numElems) {
  if (buffer_ <= allocationStart && allocationStart <= &buffer_[bufferSize_ - 1]) {
    if (writeIdx_ != allocateIdx_) {
      size_type oldIdx = writeIdx_;

      size_type requestedIndex = (allocationStart - buffer_);

      if (writeIdx_.load() != requestedIndex) {
        // Out-of-order commit
        return 0;
      } else {
        size_type newIdx = oldIdx + numElems;
        if (newIdx > bufferSize_) {
          newIdx = bufferSize_;  // TODO: After wrap-around make sure not to
                                 // overwrite the read part.
        }

        // Check if the memory to be published was previously allocated.
        if (writeIdx_.compare_exchange_strong(oldIdx, newIdx)) {
          return newIdx - oldIdx;
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
  size_type dataAvailable = writeIdx_ - readIdx_;
  if (dataAvailable == 0) {
    data = nullptr;
    return 0;
  } else {
    data = &buffer_[readIdx_];
    if (dataAvailable < len) {
      len = dataAvailable;
    }
    return len;
  }
}

AtomicRingBuffer::size_type AtomicRingBuffer::consume(const pointer_type &data, size_type len) {
  if (buffer_ <= data && data <= &buffer_[bufferSize_ - 1]) {
    // not past-the-end
    size_type currentReadIdx = readIdx_;
    size_type currentWriteIdx = writeIdx_;
    size_type endOfReadableArea = (currentReadIdx <= currentWriteIdx) ? currentWriteIdx : bufferSize_;
    size_type numElemsFreeable = endOfReadableArea - currentReadIdx;
    if (numElemsFreeable == 0) {
      return 0;
    }
    if (len > numElemsFreeable) {
      len = numElemsFreeable;
    }
    size_type newReadIdx = currentReadIdx + len;
    if (readIdx_.compare_exchange_strong(currentReadIdx, newReadIdx)) {
      return len;
    } else {
      return 0;
    }

  } else {
    return 0;
  }
}