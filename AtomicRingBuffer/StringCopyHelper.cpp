#include "AtomicRingBuffer/StringCopyHelper.h"

namespace AtomicRingBuffer {
size_t memcpyCharReplace(char*& dest, const char* src, char search, const char* replace, size_t destLen,
                         size_t srcLen) {
  // Check preconditions
  if (src == nullptr || dest == nullptr || srcLen == 0 || destLen == 0) {
    // Nothing to do.
    return 0;
  }

  std::size_t readIdx = 0;
  std::size_t writeIdx = 0;

  // Figure out how long the replacement string actually is.
  const std::size_t replaceLen = (replace != nullptr ? strlen(replace) : 0);

  while (writeIdx < destLen && readIdx < srcLen) {
    if (src[readIdx] == search) {
      if (writeIdx >= destLen - replaceLen + 1) {
        // Check if replace will fit in the buffer.
        // Break the loop if it won't fit.
        break;
      } else {
        // Does fit. Now copy the replacement string.
        memcpy(&dest[writeIdx], replace, replaceLen);
        writeIdx += replaceLen;
      }
    } else {
      dest[writeIdx] = src[readIdx];
      ++writeIdx;
    }
    ++readIdx;
  }

  // Setup return values
  dest = &dest[writeIdx];
  return readIdx;
}

}  // namespace AtomicRingBuffer
