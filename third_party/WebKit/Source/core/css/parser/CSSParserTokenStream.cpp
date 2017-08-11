// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/parser/CSSParserTokenStream.h"

namespace blink {

void CSSParserTokenStream::ConsumeWhitespace() {
  if (Peek().GetType() == kWhitespaceToken)
    UncheckedConsume();

  DCHECK_NE(Peek().GetType(), kWhitespaceToken);
}

const CSSParserToken& CSSParserTokenStream::ConsumeIncludingWhitespace() {
  const CSSParserToken& result = Consume();
  ConsumeWhitespace();
  return result;
}

void CSSParserTokenStream::UncheckedConsumeComponentValue(
    unsigned nesting_level) {
  do {
    const CSSParserToken& token = UncheckedConsumeInternal();
    if (token.GetBlockType() == CSSParserToken::kBlockStart)
      nesting_level++;
    else if (token.GetBlockType() == CSSParserToken::kBlockEnd)
      nesting_level--;
  } while (nesting_level && !PeekInternal().IsEOF());
}

const CSSParserToken& CSSParserTokenStream::UncheckedConsumeInternal() {
  DCHECK_LE(next_index_, tokenizer_.tokens_.size());
  return tokenizer_.tokens_[next_index_++];
}

const CSSParserToken& CSSParserTokenStream::PeekInternal() const {
  if (next_index_ == tokenizer_.tokens_.size()) {
    // Reached end of token buffer, but might not be end of input.
    if (tokenizer_.TokenizeSingle().IsEOF())
      return g_static_eof_token;
  }
  DCHECK_LT(next_index_, tokenizer_.tokens_.size());
  return UncheckedPeek();
}

}  // namespace blink
