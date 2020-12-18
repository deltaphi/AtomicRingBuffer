#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "AtomicRingBuffer/StringCopyHelper.h"

class StringCopyHelperFixture : public ::testing::Test {
 public:
  void SetUp() { memset(dstBuf_, kInitialValue_, kBufferSize_); }

  constexpr static const std::size_t kBufferSize_ = 128;
  constexpr static const char kInitialValue_ = 0xFF;
  using Buffer_t = char[kBufferSize_];
  Buffer_t dstBuf_;

  char* dst_ = dstBuf_;
};

TEST_F(StringCopyHelperFixture, srcNull) {
  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, nullptr, '\n', "\r\n", kBufferSize_, 25), 0);
  EXPECT_EQ(dst_, dstBuf_);
}

TEST_F(StringCopyHelperFixture, dstNull) {
  dst_ = nullptr;
  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, nullptr, '\n', "\r\n", kBufferSize_, 25), 0);
  EXPECT_EQ(dst_, nullptr);
}

TEST_F(StringCopyHelperFixture, srcLenNull) {
  constexpr static const char* text = "Hallo, Welt!";
  const std::size_t charCount = strlen(text);

  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, text, '\n', "\r\n", kBufferSize_, 0), 0);
  EXPECT_EQ(dst_, dstBuf_);
}

TEST_F(StringCopyHelperFixture, dstLenNull) {
  constexpr static const char* text = "Hallo, Welt!";
  const std::size_t charCount = strlen(text);

  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, text, '\n', "\r\n", 0, charCount), 0);
  EXPECT_EQ(dst_, dstBuf_);
}

TEST_F(StringCopyHelperFixture, replaceNull_noOccurence) {
  constexpr static const char* text = "Hallo, Welt!";
  const std::size_t charCount = strlen(text);

  EXPECT_NE(dst_, text);

  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, text, '\n', nullptr, kBufferSize_, charCount), charCount);

  EXPECT_NE(dst_, text);
  EXPECT_EQ(dst_, dstBuf_ + charCount);
  EXPECT_EQ(dst_[0], kInitialValue_);
}

TEST_F(StringCopyHelperFixture, replaceNull_occurence) {
  constexpr static const char* text = "Hallo,\n Welt!";
  const std::size_t charCount = strlen(text);

  EXPECT_NE(dst_, text);

  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, text, '\n', nullptr, kBufferSize_, charCount), charCount);

  EXPECT_NE(dst_, text);
  EXPECT_EQ(dst_, dstBuf_ + charCount - 1);
}

TEST_F(StringCopyHelperFixture, noNewline) {
  constexpr static const char* text = "Hallo, Welt!";
  const std::size_t charCount = strlen(text);

  EXPECT_NE(dst_, text);

  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, text, '\n', "\r\n", kBufferSize_, charCount), charCount);

  EXPECT_NE(dst_, text);
  EXPECT_NE(dst_, dstBuf_);

  EXPECT_EQ(memcmp(dstBuf_, text, charCount), 0);
  for (int i = charCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
}

TEST_F(StringCopyHelperFixture, newlineMiddle) {
  constexpr static const char* text = "Hallo,\n Welt!";
  const std::size_t charCount = strlen(text);

  constexpr static const char* expectedText = "Hallo,\r\n Welt!";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, text, '\n', "\r\n", kBufferSize_, charCount), charCount);

  EXPECT_NE(dst_, text);
  // EXPECT_EQ(dst_, dstBuf_ + expectedCharCount);
  EXPECT_EQ(dst_ - dstBuf_, expectedCharCount) << "dstBuf_: " << dstBuf_;

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (int i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
}

TEST_F(StringCopyHelperFixture, newlineEnd) {
  constexpr static const char* text = "Hallo, Welt!\n";
  const std::size_t charCount = strlen(text);

  constexpr static const char* expectedText = "Hallo, Welt!\r\n";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, text, '\n', "\r\n", kBufferSize_, charCount), charCount);

  EXPECT_NE(dst_, text);
  EXPECT_EQ(dst_, dstBuf_ + expectedCharCount);

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (int i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
}

TEST_F(StringCopyHelperFixture, newlineBeginning) {
  constexpr static const char* text = "\nHallo, Welt!";
  const std::size_t charCount = strlen(text);

  constexpr static const char* expectedText = "\r\nHallo, Welt!";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, text, '\n', "\r\n", kBufferSize_, charCount), charCount);

  EXPECT_NE(dst_, text);
  EXPECT_EQ(dst_, dstBuf_ + expectedCharCount);

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (int i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
}

TEST_F(StringCopyHelperFixture, multiNewline) {
  constexpr static const char* text = "Hallo,\n Welt!\n";
  const std::size_t charCount = strlen(text);

  constexpr static const char* expectedText = "Hallo,\r\n Welt!\r\n";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, text, '\n', "\r\n", kBufferSize_, charCount), charCount);

  EXPECT_NE(dst_, text);
  EXPECT_EQ(dst_, dstBuf_ + expectedCharCount);

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (int i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
}

TEST_F(StringCopyHelperFixture, multiNewlineBeginning) {
  constexpr static const char* text = "\nHallo,\n Welt!\n";
  const std::size_t charCount = strlen(text);

  constexpr static const char* expectedText = "\r\nHallo,\r\n Welt!\r\n";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, text, '\n', "\r\n", kBufferSize_, charCount), charCount);

  EXPECT_NE(dst_, text);
  EXPECT_EQ(dst_, dstBuf_ + expectedCharCount);

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (int i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
}

TEST_F(StringCopyHelperFixture, repeatedNewlineStart) {
  constexpr static const char* text = "\n\nHallo, Welt!";
  const std::size_t charCount = strlen(text);

  constexpr static const char* expectedText = "\r\n\r\nHallo, Welt!";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, text, '\n', "\r\n", kBufferSize_, charCount), charCount);

  EXPECT_NE(dst_, text);
  EXPECT_EQ(dst_, dstBuf_ + expectedCharCount);

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (int i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
}

TEST_F(StringCopyHelperFixture, repeatedNewlineMiddle) {
  constexpr static const char* text = "Hallo,\n\n Welt!";
  const std::size_t charCount = strlen(text);

  constexpr static const char* expectedText = "Hallo,\r\n\r\n Welt!";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, text, '\n', "\r\n", kBufferSize_, charCount), charCount);

  EXPECT_NE(dst_, text);
  EXPECT_EQ(dst_, dstBuf_ + expectedCharCount);

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (int i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
}

TEST_F(StringCopyHelperFixture, repeatedNewlineEnd) {
  constexpr static const char* text = "Hallo, Welt!\n\n";
  const std::size_t charCount = strlen(text);

  constexpr static const char* expectedText = "Hallo, Welt!\r\n\r\n";
  const std::size_t expectedCharCount = strlen(expectedText);

  EXPECT_STRNE(dst_, text);

  EXPECT_EQ(AtomicRingBuffer::memcpyCharReplace(dst_, text, '\n', "\r\n", kBufferSize_, charCount), charCount);

  EXPECT_NE(dst_, text);
  EXPECT_EQ(dst_, dstBuf_ + expectedCharCount);

  EXPECT_EQ(memcmp(dstBuf_, expectedText, expectedCharCount), 0);
  for (int i = expectedCharCount; i < kBufferSize_; ++i) {
    EXPECT_EQ(dstBuf_[i], kInitialValue_) << "dstBuf_[" << i << "]: " << dstBuf_[i];
  }
}
