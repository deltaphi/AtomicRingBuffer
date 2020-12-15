#ifndef __ATOMIC_RING_BUFFER__H__
#define __ATOMIC_RING_BUFFER__H__

#include <atomic>
#include <cstdint>

class AtomicRingBuffer {
 public:
  using value_type = uint8_t;
  using pointer_type = value_type *;
  using size_type = uint8_t;
  using atomic_size_type = std::atomic_uint8_t;

  AtomicRingBuffer() : buffer_(nullptr), bufferSize_(0), writeIdx_(0), allocateIdx_(0), readIdx_(0) {}

  void init(pointer_type buf, size_type len) {
    buffer_ = buf;
    bufferSize_ = len;

    writeIdx_ = 0;
    allocateIdx_ = 0;
    readIdx_ = 0;
  }

  /**
   * Get as many elements as requested  without wraparound.
   * if partial_acceptable is true, returns memory even if there are fewer
   * elements available without wraparound than were expected. Return number of
   * allocated elements.
   */
  size_type allocate(pointer_type &memory, size_type numElems, bool partial_acceptable) {
    memory = nullptr;

    // Load expected start of writable area
    size_type currentAllocateIdx = allocateIdx_.load();

    // Find out whether end of buffer area or readIdx is ahead
    size_type availableBytes;
    if (readIdx_ < currentAllocateIdx) {
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
      return numElems;
    } else {
      return 0;
    }
  }

  /**
   * Sends of allocated bytes. Can send parts of an allocaton but cannot send
   * out-of-order. Cannot send bytes wrapping around the buffer.
   */
  size_type publish(const pointer_type allocationStart, size_type numElems) {
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

      if (writeIdx_.compare_exchange_strong(oldIdx, newIdx)) {
        return newIdx - oldIdx;
      } else {
        return 0;
      }
    }
  }

  /**
   * Returns pointer and length to available data.
   */
  size_type peek(pointer_type &data, size_type len) const {
    size_type dataAvailable = writeIdx_ - readIdx_;
    data = &buffer_[readIdx_];
    if (dataAvailable < len) {
      len = dataAvailable;
    }
    return len;
  }

  /**
   * Free up space in the buffer.
   */
  size_type consume(const pointer_type &data, size_type len) {
    if (buffer_ <= data && data <= &buffer_[bufferSize_ - 1]) {
      // not past-the-end
      size_type currentReadIdx = readIdx_;
      size_type currentWriteIdx = readIdx_;
      size_type endOfReadableArea = (currentReadIdx < currentWriteIdx) ? currentWriteIdx : bufferSize_;
      size_type numElemsFreeable = endOfReadableArea - currentReadIdx;
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

 private:
  pointer_type buffer_;
  size_type bufferSize_;

  // Until where can be read
  atomic_size_type writeIdx_;

  // Until where elements have been allocated
  atomic_size_type allocateIdx_;

  // From where can be read
  atomic_size_type readIdx_;
};

#endif  // !__ATOMIC_RING_BUFFER__H__
