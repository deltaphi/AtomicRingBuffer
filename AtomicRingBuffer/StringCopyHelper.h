#ifndef __ATOMICRINGBUFFER__STRINGCOPYHELPER_H__
#define __ATOMICRINGBUFFER__STRINGCOPYHELPER_H__

#include <cstring>

namespace AtomicRingBuffer {

struct memcpyCharReplaceResult {
  size_t len = 0;
  char* nextByte = nullptr;
  const char* partialReplace = nullptr;
};

/**
 * \brief Copy the contents of src (length srcLen) to dest (destLen) while replacing every occurrenct of search in src
 * with replace in dest.
 *
 * If dest is of insufficient size, fill up as much of the result as possible.
 *
 * dest will not contain any instance of search, unless search is contained in replace. Occurrences of search in replace
 * will not be replaced again.
 *
 * If src and dest overlap, the behavior is undefined. However, it may just work if writing to dest only touches already
 * consumed parts of src.
 *
 * If dest runs out while inserting a copy of replace, replace is updated to point to the first character in replace
 * that was not copied into dest.
 *
 * \param dest Reference to pointer to the destination buffer. After the call, points to the byte after the last byte
 * that was written (which may be past the end of dest!)
 * \param src The (const!) char pointer to the bytes to read from.
 * \param search the character to be replaced. For copying without replacement, use memcpy().
 * \param replace The character sequence to insert instead of search. If this is nullptr, search is replaced with
 * nothing.
 * \param destLen The number of bytes that can be written to dest.
 * \param srcLen The number of bytes that can
 * be read from src.
 *
 * \returns The number of bytes that were consumed from src.
 *
 */
memcpyCharReplaceResult memcpyCharReplace(char* const dest, const char* const src, const char search,
                                          const char* const replace, const size_t destLen, const size_t srcLen);

}  // namespace AtomicRingBuffer

#endif  // __ATOMICRINGBUFFER__STRINGCOPYHELPER_H__
