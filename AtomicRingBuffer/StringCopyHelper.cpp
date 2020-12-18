#include "AtomicRingBuffer/StringCopyHelper.h"

namespace AtomicRingBuffer {
size_t memcpyCharReplace(char*& dest, const char* src, char search, const char* replace, size_t destLen,
                         size_t srcLen) {
  if (src == nullptr || dest == nullptr || srcLen == 0 || destLen == 0) {
    // Nothing to do.
    return 0;
  }

  /*
    // Count number of newlines
  const char* lastNewLine = src;
  const char* nextNewLine = src;
  std::size_t numSearchFinds = 0;
  std::size_t bytesAnalyzed = 0;
  while (nextNewLine != nullptr) {
    nextNewLine = static_cast<const char*>(memchr(nextNewLine, search, srcLen - bytesAnalyzed));
    if (nextNewLine != nullptr) {
      ++numSearchFinds;
      bytesAnalyzed += nextNewLine - lastNewLine;
      lastNewLine = nextNewLine;
      ++nextNewLine;
    }
  }
  */

  std::size_t readIdx = 0;
  std::size_t writeIdx = 0;

  const std::size_t replaceLen = (replace != nullptr ? strlen(replace) : 0);

  // TODO: If ptr[readIdx] == '\n' we need two bytes to be available!
  while (writeIdx < destLen && readIdx < srcLen) {
    if (src[readIdx] == search) {
      if (writeIdx >= destLen - replaceLen + 1) {
        // Check if replace will fit in the buffer.
        // Break the loop if it won't fit.
        break;
      } else {
        // Does fit. Now copy.
        memcpy(&dest[writeIdx], replace, replaceLen);
        writeIdx += replaceLen;
      }
    } else {
      dest[writeIdx] = src[readIdx];
      ++writeIdx;
    }
    ++readIdx;
  }

  dest = &dest[writeIdx];
  return readIdx;
}

}  // namespace AtomicRingBuffer
