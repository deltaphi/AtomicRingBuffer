#include "AtomicRingBuffer/StringCopyHelper.h"

#include <algorithm>

namespace AtomicRingBuffer {

memcpyCharReplaceResult memcpyCharReplace(char* dest, const char* src, const char search, const char* const replace,
                                          const size_t destLen, const size_t srcLen) {
  memcpyCharReplaceResult result;

  // Check preconditions
  if (src == nullptr || dest == nullptr || srcLen == 0 || destLen == 0) {
    // Nothing to do.
    return result;
  }

  const char* const srcEnd = std::next(src, srcLen);
  char* const destEnd = std::next(dest, destLen);

  // Figure out how long the replacement string actually is.
  const std::size_t replaceLen = (replace != nullptr ? strlen(replace) : 0);

  const char* const srcBegin = src;
  const char* nextSearchOccurence = src;

  while (dest < destEnd && src < srcEnd) {
    if (nextSearchOccurence != nullptr) {
      nextSearchOccurence = static_cast<const char*>(memchr(nextSearchOccurence, search, std::distance(src, srcEnd)));
    }
    const bool searchFound = (nextSearchOccurence != nullptr);

    // Copy over all data from readIdx until a position where search was found or until the end of src, otherwise.
    // Copy is limited to the length of the output buffer.
    {
      const char* transferEnd = (searchFound) ? nextSearchOccurence : srcEnd;
      const std::ptrdiff_t srcBytesAvailable = std::distance(src, transferEnd);
      const std::ptrdiff_t destBytesAvailable = std::distance(dest, destEnd);

      // Copy everything from lastSearchResult to whatever will fit in the remaining buffer.
      const std::ptrdiff_t bytesToTransfer = std::min(srcBytesAvailable, destBytesAvailable);
      memcpy(dest, src, bytesToTransfer);
      std::advance(src, bytesToTransfer);
      std::advance(dest, bytesToTransfer);
    }

    // If search was found, insert replace.
    if (searchFound) {
      const std::size_t bytesRemainingInBuffer = std::distance(dest, destEnd);
      if (bytesRemainingInBuffer > 0) {
        const std::size_t bytesToInsert = std::min(replaceLen, bytesRemainingInBuffer);

        if (bytesToInsert > 0) {
          memcpy(dest, replace, bytesToInsert);
          std::advance(dest, bytesToInsert);
        }

        const bool resultPartiallyWritten = bytesToInsert != replaceLen;
        if (resultPartiallyWritten) {
          // Advance replace to the next unwritten byte
          result.partialReplace = std::next(replace, bytesToInsert);
        }

        // Then continue with the source byte afterwards.
        std::advance(src, 1);
        std::advance(nextSearchOccurence, 1);
      }
    }
  }

  // Setup return values
  result.len = std::distance(srcBegin, src);
  result.nextByte = dest;

  return result;
}

}  // namespace AtomicRingBuffer
