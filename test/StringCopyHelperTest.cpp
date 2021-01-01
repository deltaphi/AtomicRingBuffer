#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "AtomicRingBuffer/StringCopyHelper.h"

class StringCopyHelperFixture : public ::testing::Test {
 public:
  void SetUp() { memset(dstBuf_, kInitialValue_, kBufferSize_); }

  constexpr static const std::size_t kBufferSize_ = 128;
  constexpr static const char kInitialValue_ = '\xFF';
  using Buffer_t = char[kBufferSize_];
  Buffer_t dstBuf_;

  char* dst_ = dstBuf_;

  constexpr static const char search = '\n';
  constexpr static const char* replace = "\r\n";
};

const std::size_t StringCopyHelperFixture::kBufferSize_;
const char StringCopyHelperFixture::kInitialValue_;
const char StringCopyHelperFixture::search;
constexpr const char* StringCopyHelperFixture::replace;

TEST_F(StringCopyHelperFixture, srcNull) {
  const char* replacePtr = replace;
  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, nullptr, search, replacePtr, kBufferSize_, 25);
  EXPECT_EQ(result.len, 0);
  EXPECT_EQ(result.nextByte, nullptr);
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, dstNull) {
  const char* replacePtr = replace;
  dst_ = nullptr;
  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, nullptr, search, replacePtr, kBufferSize_, 25);
  EXPECT_EQ(result.len, 0);
  EXPECT_EQ(result.nextByte, nullptr);
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, srcLenNull) {
  const char* replacePtr = replace;

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, nullptr, search, replacePtr, kBufferSize_, 0);
  EXPECT_EQ(result.len, 0);
  EXPECT_EQ(result.nextByte, nullptr);
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, dstLenNull) {
  constexpr static const char* text = "Hallo, Welt!";
  const std::size_t charCount = strlen(text);
  const char* replacePtr = replace;

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, 0, charCount);
  EXPECT_EQ(result.len, 0);
  EXPECT_EQ(result.nextByte, nullptr);
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, replaceNull_noOccurence) {
  constexpr static const char* text = "Hallo, Welt!";
  const std::size_t charCount = strlen(text);
  const char* replacePtr = nullptr;

  EXPECT_NE(dst_, text);

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, charCount);

  EXPECT_NE(result.nextByte, text);
  EXPECT_EQ(result.nextByte, dstBuf_ + charCount);
  EXPECT_EQ(result.nextByte[0], kInitialValue_);
  EXPECT_EQ(replacePtr, nullptr);
}

TEST_F(StringCopyHelperFixture, replaceNull_occurence) {
  constexpr static const char* text = "Hallo,\n Welt!";
  const std::size_t charCount = strlen(text);
  const char* replacePtr = nullptr;

  EXPECT_NE(dst_, text);

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, charCount);

  EXPECT_NE(result.nextByte, text);
  EXPECT_EQ(result.nextByte, dstBuf_ + charCount - 1);
  EXPECT_EQ(replacePtr, nullptr);
}

TEST_F(StringCopyHelperFixture, noNewline) {
  constexpr static const char* text = "Hallo, Welt!";
  const std::size_t charCount = strlen(text);
  const char* replacePtr = replace;

  EXPECT_NE(dst_, text);

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, charCount);

  EXPECT_NE(result.nextByte, text);
  EXPECT_NE(result.nextByte, dstBuf_);

  EXPECT_EQ(memcmp(dstBuf_, text, charCount), 0);
  for (std::size_t i = charCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, newlineMiddle) {
  constexpr static const char* text = "Hallo,\n Welt!";
  const std::size_t charCount = strlen(text);
  const char* replacePtr = replace;

  constexpr static const char* expectedText = "Hallo,\r\n Welt!";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, charCount);

  EXPECT_NE(result.nextByte, text);
  EXPECT_EQ(result.nextByte - dstBuf_, expectedCharCount) << "dstBuf_: " << dstBuf_;

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (std::size_t i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, newlineEnd) {
  constexpr static const char* text = "Hallo, Welt!\n";
  const std::size_t charCount = strlen(text);
  const char* replacePtr = replace;

  constexpr static const char* expectedText = "Hallo, Welt!\r\n";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, charCount);

  EXPECT_NE(result.nextByte, text);
  EXPECT_EQ(result.nextByte, dstBuf_ + expectedCharCount);

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (std::size_t i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, newlineBeginning) {
  constexpr static const char* text = "\nHallo, Welt!";
  const std::size_t charCount = strlen(text);
  const char* replacePtr = replace;

  constexpr static const char* expectedText = "\r\nHallo, Welt!";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, charCount);

  EXPECT_NE(result.nextByte, text);
  EXPECT_EQ(result.nextByte, dstBuf_ + expectedCharCount);

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (std::size_t i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, multiNewline) {
  constexpr static const char* text = "Hallo,\n Welt!\n";
  const std::size_t charCount = strlen(text);
  const char* replacePtr = replace;

  constexpr static const char* expectedText = "Hallo,\r\n Welt!\r\n";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, charCount);

  EXPECT_NE(result.nextByte, text);
  EXPECT_EQ(result.nextByte, dstBuf_ + expectedCharCount);

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (std::size_t i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, multiNewlineBeginning) {
  constexpr static const char* text = "\nHallo,\n Welt!\n";
  const std::size_t charCount = strlen(text);
  const char* replacePtr = replace;

  constexpr static const char* expectedText = "\r\nHallo,\r\n Welt!\r\n";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, charCount);

  EXPECT_NE(result.nextByte, text);
  EXPECT_EQ(result.nextByte, dstBuf_ + expectedCharCount);

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (std::size_t i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, repeatedNewlineStart) {
  constexpr static const char* text = "\n\nHallo, Welt!";
  const std::size_t charCount = strlen(text);
  const char* replacePtr = replace;

  constexpr static const char* expectedText = "\r\n\r\nHallo, Welt!";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, charCount);

  EXPECT_NE(result.nextByte, text);
  EXPECT_EQ(result.nextByte, dstBuf_ + expectedCharCount);

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (std::size_t i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, repeatedNewlineMiddle) {
  constexpr static const char* text = "Hallo,\n\n Welt!";
  const std::size_t charCount = strlen(text);
  const char* replacePtr = replace;

  constexpr static const char* expectedText = "Hallo,\r\n\r\n Welt!";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, charCount);

  EXPECT_NE(result.nextByte, text);
  EXPECT_EQ(result.nextByte, dstBuf_ + expectedCharCount);

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (std::size_t i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, repeatedNewlineEnd) {
  constexpr static const char* text = "Hallo, Welt!\n\n";
  const std::size_t charCount = strlen(text);
  const char* replacePtr = replace;

  constexpr static const char* expectedText = "Hallo, Welt!\r\n\r\n";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, charCount);

  EXPECT_NE(result.nextByte, text);
  EXPECT_EQ(result.nextByte, dstBuf_ + expectedCharCount);

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (std::size_t i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
}

TEST_F(StringCopyHelperFixture, oversizedSource_noNewline) {
  constexpr static const std::size_t charCount = kBufferSize_ + 5;
  char text[charCount];
  const char* replacePtr = replace;

  for (std::size_t i = 0; i < charCount; ++i) {
    text[i] = ('a' + (i % 26));
  }

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, kBufferSize_);

  for (std::size_t i = 0; i < kBufferSize_; ++i) {
    EXPECT_EQ(text[i], ('a' + (i % 26)));
  }
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, oversizedSource_newlineMiddle) {
  constexpr static const std::size_t charCount = kBufferSize_ + 5;
  char text[charCount];
  const char* replacePtr = replace;

  constexpr static const std::size_t newLineIndex = kBufferSize_ / 2;
  for (std::size_t i = 0; i < charCount; ++i) {
    if (i == newLineIndex) {
      text[i] = search;
    } else {
      text[i] = ('a' + (i % 26));
    }
  }

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, kBufferSize_ - 1);

  EXPECT_EQ(result.nextByte, dstBuf_ + kBufferSize_);

  for (std::size_t i = 0; i < newLineIndex; ++i) {
    EXPECT_EQ(dstBuf_[i], ('a' + (i % 26))) << "i: " << std::dec << static_cast<int16_t>(i);
  }
  EXPECT_EQ(dstBuf_[newLineIndex], '\r');
  EXPECT_EQ(dstBuf_[newLineIndex + 1], search);
  for (std::size_t i = newLineIndex + 2; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], ('a' + ((i - 1) % 26))) << "i: " << std::dec << static_cast<int16_t>(i);
  }
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, oversizedSource_newlineBeforeBorder) {
  // a newline that just fits inside
  constexpr static const std::size_t charCount = kBufferSize_ + 5;
  char text[charCount];
  const char* replacePtr = replace;

  constexpr static const std::size_t newLineIndex = kBufferSize_ - 2;
  for (std::size_t i = 0; i < charCount; ++i) {
    if (i == newLineIndex) {
      text[i] = search;
    } else {
      text[i] = ('a' + (i % 26));
    }
  }

  // Copy everything up to the newline but not the newline itself.
  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, kBufferSize_ - 1);

  EXPECT_EQ(result.nextByte, dstBuf_ + kBufferSize_);

  for (std::size_t i = 0; i < newLineIndex; ++i) {
    EXPECT_EQ(dstBuf_[i], ('a' + (i % 26))) << "i: " << std::dec << static_cast<int16_t>(i);
  }
  EXPECT_EQ(dstBuf_[newLineIndex], '\r');
  EXPECT_EQ(dstBuf_[newLineIndex + 1], search);
  for (std::size_t i = newLineIndex + 2; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], ('a' + ((i - 1) % 26))) << "i: " << std::dec << static_cast<int16_t>(i);
  }
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, oversizedSource_newlinePastBorder) {
  // a newline that is the next character after the border
  constexpr static const std::size_t charCount = kBufferSize_ + 5;
  char text[charCount];
  const char* replacePtr = replace;

  constexpr static const std::size_t newLineIndex = charCount - 1;
  for (std::size_t i = 0; i < charCount; ++i) {
    text[i] = ('a' + (i % 26));
  }

  text[newLineIndex] = search;

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, kBufferSize_);

  EXPECT_EQ(result.nextByte, dstBuf_ + kBufferSize_);

  for (std::size_t i = 0; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], ('a' + (i % 26))) << "i: " << std::dec << static_cast<int16_t>(i);
  }
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, oversizedSource_newlineOnBorder) {
  // a newline that spans the border
  constexpr static const std::size_t charCount = kBufferSize_ + 5;
  char text[charCount];
  const char* replacePtr = replace;

  constexpr static const std::size_t newLineIndex = kBufferSize_ - 1;
  for (std::size_t i = 0; i < charCount; ++i) {
    if (i == newLineIndex) {
      text[i] = search;
    } else {
      text[i] = ('a' + (i % 26));
    }
  }

  // Copy everything up to the newline but not the newline itself.
  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, kBufferSize_);

  EXPECT_EQ(result.nextByte, dstBuf_ + kBufferSize_);

  for (std::size_t i = 0; i < kBufferSize_ - 1; ++i) {
    EXPECT_EQ(dstBuf_[i], ('a' + (i % 26))) << "i: " << std::dec << static_cast<int16_t>(i);
  }
  EXPECT_EQ(dstBuf_[kBufferSize_ - 1], replace[0]);
  EXPECT_EQ(result.partialReplace, replace + 1);
}

TEST_F(StringCopyHelperFixture, oversizedSource_MultiNewlineOnBorder) {
  // Multiple newlines around the border
  constexpr static const std::size_t charCount = kBufferSize_ + 5;
  char text[charCount];
  const char* replacePtr = replace;

  for (std::size_t i = 0; i < charCount; ++i) {
    text[i] = ('a' + (i % 26));
  }

  constexpr static const std::size_t newline1 = kBufferSize_ - 2;
  constexpr static const std::size_t newline2 = kBufferSize_ - 1;
  constexpr static const std::size_t newline3 = kBufferSize_;

  text[newline1] = search;
  text[newline2] = search;
  text[newline3] = search;

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, kBufferSize_ - 1);

  for (std::size_t i = 0; i < newline1; ++i) {
    EXPECT_EQ(dstBuf_[i], ('a' + (i % 26))) << "i: " << std::dec << static_cast<int16_t>(i);
  }
  EXPECT_EQ(dstBuf_[newline1], '\r');
  EXPECT_EQ(dstBuf_[newline1 + 1], search);
  EXPECT_EQ(result.partialReplace, nullptr);
}

TEST_F(StringCopyHelperFixture, oversizedSource_PushNewlineToBorder) {
  // a newline early in the string that pushes a newline across the border
  constexpr static const std::size_t charCount = kBufferSize_ + 5;
  char text[charCount];
  const char* replacePtr = replace;

  for (std::size_t i = 0; i < charCount; ++i) {
    text[i] = ('a' + (i % 26));
  }

  constexpr static const std::size_t newline1 = kBufferSize_ / 2;
  constexpr static const std::size_t newline2 = kBufferSize_ - 2;

  // First test: Only the second newline is active
  text[newline2] = search;

  auto result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, kBufferSize_ - 1);

  for (std::size_t i = 0; i < kBufferSize_ - 2; ++i) {
    EXPECT_EQ(dstBuf_[i], ('a' + (i % 26))) << "i: " << std::dec << static_cast<int16_t>(i);
  }
  EXPECT_EQ(dstBuf_[kBufferSize_ - 2], '\r');
  EXPECT_EQ(dstBuf_[kBufferSize_ - 1], search);
  EXPECT_EQ(result.partialReplace, nullptr);
  EXPECT_EQ(result.nextByte, dstBuf_ + kBufferSize_);

  // second test: Both newlines are active.
  text[newline1] = search;
  replacePtr = replace;

  memset(dstBuf_, kInitialValue_, kBufferSize_);
  dst_ = dstBuf_;

  result = AtomicRingBuffer::memcpyCharReplace(dst_, text, search, replacePtr, kBufferSize_, charCount);
  EXPECT_EQ(result.len, kBufferSize_ - 1);

  for (std::size_t i = 0; i < newline1; ++i) {
    EXPECT_EQ(dstBuf_[i], ('a' + (i % 26))) << "i: " << std::dec << static_cast<int16_t>(i);
  }
  EXPECT_EQ(dstBuf_[newline1], '\r');
  EXPECT_EQ(dstBuf_[newline1 + 1], search);
  for (std::size_t i = newline1 + 2; i < kBufferSize_ - 1; ++i) {
    EXPECT_EQ(dstBuf_[i], ('a' + ((i - 1) % 26))) << "i: " << std::dec << static_cast<int16_t>(i);
  }
  EXPECT_EQ(dstBuf_[kBufferSize_ - 1], replace[0]);
  EXPECT_EQ(result.nextByte, dstBuf_ + kBufferSize_);
  EXPECT_EQ(result.partialReplace, replace + 1);
}
