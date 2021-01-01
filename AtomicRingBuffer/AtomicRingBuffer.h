#ifndef __ATOMIC_RING_BUFFER__H__
#define __ATOMIC_RING_BUFFER__H__

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace AtomicRingBuffer {

/**
 * \brief Manages a round-robin buffer of bytes.
 *
 * Bytes must be allocated, can then be written to and must then be published. Bytes that have been published must be
 * peeked to be read and must finally be consumed to free up buffer space.
 *
 * Special implementation notes:
 * * The index range is twice as large as the actual buffer. This lets indices carry information whether the buffer is
 *   full or empty.
 * * Class invariant: When adjusting for the circular nature of the index range (2*bufferSize_), it holds that:
 *   readIdx <= writeIdx <= allocateIdx.
 */
class AtomicRingBuffer {
 public:
  using value_type = uint8_t;
  using pointer_type = value_type *;
  using size_type = std::size_t;
  using atomic_size_type = std::atomic_size_t;

  struct MemoryRange {
    pointer_type ptr = nullptr;
    size_type len = 0;

    bool operator==(const MemoryRange &other) const { return ptr == other.ptr && len == other.len; }
  };

  constexpr AtomicRingBuffer() : buffer_(nullptr), bufferSize_(0) {}
  constexpr AtomicRingBuffer(pointer_type buf, size_type len) : buffer_(buf), bufferSize_(len) {}

  void init(pointer_type buf, const size_type len) {
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
  MemoryRange allocate(const size_type numElems, const bool partial_acceptable);

  /**
   * Sends of allocated bytes. Can send parts of an allocaton but cannot send
   * out-of-order. Cannot send bytes wrapping around the buffer.
   */
  size_type publish(const MemoryRange data) { return commit(writeIdx_, allocateIdx_, data); }

  /**
   * Returns pointer and length to available data.
   */
  MemoryRange peek(const size_type len, const bool partial_acceptable) const {
    return allocate(readIdx_, writeIdx_, true, len, partial_acceptable);
  }

  /**
   * Free up space in the buffer.
   */
  size_type consume(const MemoryRange data) { return commit(readIdx_, writeIdx_, data); }

  size_type capacity() const { return bufferSize_; }

  size_type size() const {
    size_type bytesAvailable = 0;
    size_type currentReadIdx = readIdx_;
    size_type currentWriteIdx = writeIdx_;
    if (currentWriteIdx >= currentReadIdx) {
      bytesAvailable = currentWriteIdx - currentReadIdx;
    } else {
      bytesAvailable = bytesRemainingInBuffer(currentReadIdx);
      bytesAvailable += wrapToBufferSize(currentWriteIdx);
    }

    return bytesAvailable;
  }

 private:
  constexpr size_type beginIdx() const { return 0; }
  constexpr size_type endIdx() const { return bufferSize_; }
  constexpr size_type upperSectionEndIdx() const { return 2 * bufferSize_; }

  constexpr size_type wrapToBufferSize(size_type idx) const {
    if (idx > bufferSize_) {
      idx -= bufferSize_;
    }
    return idx;
  }

  constexpr size_type wrapToBufferIdx(size_type idx) const {
    if (idx >= bufferSize_) {
      idx -= bufferSize_;
    }
    return idx;
  }

  constexpr size_type wrapToDoubleBufferIdx(size_type idx) const {
    if (idx >= (2 * bufferSize_)) {
      idx -= 2 * bufferSize_;
    }
    return idx;
  }

  size_type bytesToPointerOrBufferEnd(const size_type lower, const size_type upper, bool isInside) const;
  size_type bytesToPointerOrBufferEnd_inside(const size_type lower, const size_type upper) const {
    return bytesToPointerOrBufferEnd(lower, upper, true);
  }

  constexpr size_type bytesRemainingInBuffer(const size_type idx) const {
    if (idxInUpperSection(idx)) {
      return upperSectionEndIdx() - idx;
    } else {
      return endIdx() - idx;
    }
  }

  MemoryRange allocate(const size_type sectionBegin, const size_type sectionEnd, const bool isInside,
                       const size_type len, const bool partial_acceptable) const;
  size_type commit(atomic_size_type &sectionBegin, atomic_size_type &sectionEnd, const MemoryRange data);

  /**
   * \brief Whether a pointer points to the lower or the upper round of the buffer
   *
   * \returns FALSE for lower round, TRUE for upper round.
   */
  constexpr bool idxInUpperSection(const size_type idx) const { return idx >= endIdx(); }

  constexpr bool sameSection(const size_type lower, const size_type upper) const {
    return idxInUpperSection(lower) == idxInUpperSection(upper);
  }

  pointer_type buffer_;
  size_type bufferSize_;

  // Until where can be read
  atomic_size_type writeIdx_{0};

  // Until where elements have been allocated
  atomic_size_type allocateIdx_{0};

  // From where can be read
  atomic_size_type readIdx_{0};
};

}  // namespace AtomicRingBuffer

#endif  // !__ATOMIC_RING_BUFFER__H__
