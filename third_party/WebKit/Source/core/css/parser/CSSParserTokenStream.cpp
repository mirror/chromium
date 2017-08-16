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

CSSParserToken CSSParserTokenStream::ConsumeIncludingWhitespace() {
  CSSParserToken result = Consume();
  ConsumeWhitespace();
  return result;
}

void CSSParserTokenStream::UncheckedConsumeComponentValue(
    unsigned nesting_level) {
  DCHECK_LT(next_index_, tokenizer_.CurrentSize());

  // Can't use consume/peek in here because they can't read past start/end of
  // blocks
  do {
    const CSSParserToken& token = tokenizer_.tokens_[next_index_++];
    if (token.GetBlockType() == CSSParserToken::kBlockStart)
      nesting_level++;
    else if (token.GetBlockType() == CSSParserToken::kBlockEnd)
      nesting_level--;

    if (EnsureLookAhead() == LookAhead::kIsEOF)
      return;

    DCHECK_LT(next_index_, tokenizer_.CurrentSize());
  } while (nesting_level && !tokenizer_.tokens_[next_index_].IsEOF());
}

}  // namespace blink
