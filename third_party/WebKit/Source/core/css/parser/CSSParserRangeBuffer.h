// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSParserRangeBuffer_h
#define CSSParserRangeBuffer_h

#include "core/css/parser/CSSParserTokenStream.h"

namespace blink {

class CSSParserRangeBuffer {
  WTF_MAKE_NONCOPYABLE(CSSParserRangeBuffer);

 public:
  CSSParserRangeBuffer(CSSParserTokenStream& stream)
      : buffer_(stream.buffer_), starting_buffer_size_(buffer_.size()) {}

  CSSParserRangeBuffer(CSSParserRangeBuffer&& other)
      : buffer_(other.buffer_),
        starting_buffer_size_(other.starting_buffer_size_),
        num_tokens_(other.num_tokens_) {
    other.starting_buffer_size_ = 0;
    other.num_tokens_ = 0;
  }

  ~CSSParserRangeBuffer() {
    if (num_tokens_ == 0)
      return;

    DCHECK_EQ(buffer_.size(), starting_buffer_size_ + num_tokens_);
    buffer_.resize(starting_buffer_size_);
  }

  void Append(const CSSParserToken& token) {
    DCHECK_EQ(buffer_.size(), starting_buffer_size_ + num_tokens_);
    buffer_.push_back(token);
    ++num_tokens_;
  }

  CSSParserTokenRange Range() {
    const auto begin = buffer_.Tokens().begin();
    return CSSParserTokenRange(buffer_.Tokens())
        .MakeSubRange(begin + starting_buffer_size_,
                      begin + starting_buffer_size_ + num_tokens_);
  }

  void Release() {
    DCHECK_EQ(buffer_.size(), starting_buffer_size_ + num_tokens_);
    buffer_.resize(starting_buffer_size_);

    // Prevent the destructor from releasing this again
    num_tokens_ = 0;
    starting_buffer_size_ = 0;
  }

 private:
  CSSTokenizerBuffer& buffer_;
  size_t starting_buffer_size_;
  size_t num_tokens_ = 0;
};

}  // namespace blink

#endif  // CSSParserRangeBuffer_h
