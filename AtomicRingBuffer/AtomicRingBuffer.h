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
  size_type allocate(pointer_type &memory, size_type numElems, bool partial_acceptable);

  /**
   * Sends of allocated bytes. Can send parts of an allocaton but cannot send
   * out-of-order. Cannot send bytes wrapping around the buffer.
   */
  size_type publish(const pointer_type allocationStart, size_type numElems);

  /**
   * Returns pointer and length to available data.
   */
  size_type peek(pointer_type &data, size_type len, bool partial_acceptable) const;

  /**
   * Free up space in the buffer.
   */
  size_type consume(const pointer_type &data, size_type len);

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
