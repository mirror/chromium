// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSParserRangeBuffer_h
#define CSSParserRangeBuffer_h

#include "core/css/parser/CSSParserTokenStream.h"

namespace blink {

// Represents a vector of CSSParserTokens, used to create CSSParserTokenRanges.
// This class owns the tokens that it stores, destroying them when it goes out
// of scope.
//
// Currently, only one CSSParserRangeBuffer can be active at any point in time
// per stream.
// TODO(shend): Allow multiple CSSParserRangeBuffers to be active by making
// CSSParserTokenStream store a linked list of Vectors.
class CSSParserRangeBuffer {
  WTF_MAKE_NONCOPYABLE(CSSParserRangeBuffer);

 public:
  CSSParserRangeBuffer(CSSParserTokenStream& stream) : buffer_(stream.buffer_) {
    DCHECK_EQ(buffer_.size(), 0U);
  }

  CSSParserRangeBuffer(CSSParserRangeBuffer&& other)
      : buffer_(other.buffer_), num_tokens_(other.num_tokens_) {
    other.num_tokens_ = 0;
  }

  ~CSSParserRangeBuffer() { Release(); }

  // This invalidates any ranges created from this buffer.
  void Append(const CSSParserToken& token) {
    DCHECK_EQ(buffer_.size(), num_tokens_);
    buffer_.push_back(token);
    ++num_tokens_;
  }

  CSSParserTokenRange Range() {
    const auto begin = buffer_.Tokens().begin();
    return CSSParserTokenRange(buffer_.Tokens())
        .MakeSubRange(begin, begin + num_tokens_);
  }

  void Release() {
    if (num_tokens_ == 0)
      return;

    DCHECK_EQ(buffer_.size(), num_tokens_);
    buffer_.clear();

    // Prevent double releasing
    num_tokens_ = 0;
  }

 private:
  CSSTokenizerBuffer& buffer_;
  size_t num_tokens_ = 0;
};

}  // namespace blink

#endif  // CSSParserRangeBuffer_h
