#include "AtomicRingBuffer/StringCopyHelper.h"

#include <algorithm>

namespace AtomicRingBuffer {

memcpyCharReplaceResult memcpyCharReplace(char* const dest, const char* const src, const char search,
                                          const char* const replace, const size_t destLen, const size_t srcLen) {
  memcpyCharReplaceResult result;

  // Check preconditions
  if (src == nullptr || dest == nullptr || srcLen == 0 || destLen == 0) {
    // Nothing to do.
    return result;
  }

  std::size_t readIdx = 0;
  std::size_t writeIdx = 0;

  // Figure out how long the replacement string actually is.
  const std::size_t replaceLen = (replace != nullptr ? strlen(replace) : 0);

  const char* nextSearchOccurence = src;

  while (writeIdx < destLen && readIdx < srcLen) {
    nextSearchOccurence = static_cast<const char*>(memchr(nextSearchOccurence, search, srcLen - readIdx));

    // Copy over all data from readIdx until a position where search was found or until the end of src, otherwise.
    // Copy is limited to the length of the output buffer.
    {
      std::size_t bytesToTransfer;

      if (nextSearchOccurence == nullptr) {
        // If search has not been found, consume from readIdx until the end.
        bytesToTransfer = srcLen - readIdx;
      } else {
        // If search has been found, consume from src[readIdx] until nextSearchResult - 1
        bytesToTransfer = (nextSearchOccurence - src) - readIdx;
      }

      // Copy everything from lastSearchResult to whatever will fit in the remaining buffer.
      bytesToTransfer = std::min(bytesToTransfer, destLen - writeIdx);
      memcpy(&dest[writeIdx], &src[readIdx], bytesToTransfer);
      readIdx += bytesToTransfer;
      writeIdx += bytesToTransfer;
    }

    // If search was found, insert replace.
    if (nextSearchOccurence != nullptr) {
      std::size_t bytesRemainingInBuffer = destLen - writeIdx;
      if (bytesRemainingInBuffer > 0) {
        std::size_t bytesToInsert = std::min(replaceLen, bytesRemainingInBuffer);

        memcpy(&dest[writeIdx], replace, bytesToInsert);

        writeIdx += bytesToInsert;

        bool resultPartiallyWritten = bytesToInsert != replaceLen;
        if (resultPartiallyWritten) {
          // Advance replace to the next unwritten byte
          result.partialReplace = &replace[bytesToInsert];
        }

        // Then continue with the source byte afterwards.
        ++readIdx;
        ++nextSearchOccurence;
      }
    }
  }

  // Setup return values
  result.len = readIdx;
  result.nextByte = &dest[writeIdx];

  return result;
}

}  // namespace AtomicRingBuffer
