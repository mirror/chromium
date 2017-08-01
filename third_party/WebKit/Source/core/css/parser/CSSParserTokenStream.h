// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSParserTokenStream_h
#define CSSParserTokenStream_h

#include "CSSTokenizer.h"

namespace blink {

class CSSParserTokenStream {
  DISALLOW_NEW();

 public:
  explicit CSSParserTokenStream(CSSTokenizer& tokenizer)
      : tokenizer_(tokenizer), current_position_(0) {
    DCHECK_EQ(tokenizer.tokens_.size(), 0U);
  }

  const CSSParserToken& Peek() const {
    if (current_position_ == tokenizer_.tokens_.size()) {
      if (tokenizer_.TokenizeSingle().IsEOF())
        return g_static_eof_token;
    }
    DCHECK_EQ(current_position_, tokenizer_.tokens_.size() - 1);
    return tokenizer_.tokens_.back();
  }

  const CSSParserToken& Consume() {
    const CSSParserToken& token = Peek();
    if (!token.IsEOF()) {
      current_position_++;
    }

    DCHECK_EQ(current_position_, tokenizer_.tokens_.size());
    return token;
  }

  bool AtEnd() const { return Peek().IsEOF(); }

  CSSParserTokenRange MakeRangeToEOF() { return tokenizer_.TokenRange(); }

 private:
  CSSTokenizer& tokenizer_;
  size_t current_position_;
};

}  // namespace blink

#endif  // CSSParserTokenStream_h
