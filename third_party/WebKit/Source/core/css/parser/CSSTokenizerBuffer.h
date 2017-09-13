// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSTokenizerBuffer_h
#define CSSTokenizerBuffer_h

namespace blink {

// Represents a last-in-first-out buffer of CSSParserTokens.
class CSSTokenizerBuffer {
 public:
  CSSTokenizerBuffer(size_t reserve = 0) : size_(0) {
    tokens_.ReserveInitialCapacity(reserve);
  }

  void push_back(const CSSParserToken& token) {
    if (size_ == tokens_.size())
      tokens_.push_back(token);
    else
      tokens_[size_] = token;
    size_++;
  }

  void resize(size_t size) { size_ = size; }

  size_t size() const { return size_; }

  const Vector<CSSParserToken, 32>& Tokens() const { return tokens_; }

 private:
  Vector<CSSParserToken, 32> tokens_;
  size_t size_;
};

}  // namespace blink

#endif  // CSSTokenizerBuffer_h
