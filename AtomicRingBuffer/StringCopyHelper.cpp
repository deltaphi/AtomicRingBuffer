#include "AtomicRingBuffer/StringCopyHelper.h"

#include <algorithm>

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

  const char* nextSearchResult = src;

  while (writeIdx < destLen && readIdx < srcLen) {
    nextSearchResult = static_cast<const char*>(memchr(nextSearchResult, search, srcLen - readIdx));

    std::size_t bytesToTransfer;

    if (nextSearchResult == nullptr) {
      // If nothing has been found, consume from lastSearchResult until the end.
      bytesToTransfer = srcLen - readIdx;
    } else {
      // If something has been found, consume from src[readIdx] until nextSearchResult - 1

      bytesToTransfer = (nextSearchResult - src) - readIdx;
    }

    // Copy everything from lastSearchResult to whatever will fit in the remaining buffer.
    bytesToTransfer = std::min(bytesToTransfer, destLen - writeIdx);
    memcpy(&dest[writeIdx], &src[readIdx], bytesToTransfer);
    readIdx += bytesToTransfer;
    writeIdx += bytesToTransfer;

    if (nextSearchResult != nullptr) {
      // then insert replace (unless buffer is full)
      if (writeIdx >= destLen - replaceLen + 1) {
        // Buffer full, stop working.
        break;
      } else {
        // Does fit. Now copy the replacement string.
        memcpy(&dest[writeIdx], replace, replaceLen);
        writeIdx += replaceLen;
      }

      // Then continue with the source byte afterwards.
      ++readIdx;
      ++nextSearchResult;
    }
  }

  // Setup return values
  dest = &dest[writeIdx];
  return readIdx;
}

}  // namespace AtomicRingBuffer
