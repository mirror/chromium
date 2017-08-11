// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSParserTokenStream_h
#define CSSParserTokenStream_h

#include "CSSTokenizer.h"
#include "core/css/parser/CSSParserTokenRange.h"
#include "platform/wtf/Noncopyable.h"

namespace blink {

// A streaming interface to CSSTokenizer that tokenizes on demand.
// Abstractly, the stream ends at either EOF or the beginning/end of a block.
// To consume a block, a BlockGuard must be created first to ensure that
// we finish consuming a block even if there was an error.
//
// Methods prefixed with "Unchecked" can only be called after Peek()
// returns a non-EOF token or after AtEnd() returns false, with no
// subsequent modifications to the stream such as a consume.
class CORE_EXPORT CSSParserTokenStream {
  DISALLOW_NEW();
  WTF_MAKE_NONCOPYABLE(CSSParserTokenStream);

 public:
  class Iterator {
    explicit Iterator(size_t index) : index_(index) {}

    size_t index_;
    friend class CSSParserTokenStream;
  };

  // Instantiate this to start reading from a block. When the guard is out of
  // scope, the rest of the block is consumed.
  class BlockGuard {
   public:
    explicit BlockGuard(CSSParserTokenStream& stream) : stream_(stream) {
      DCHECK(stream_.Peek().GetBlockType() == CSSParserToken::kBlockStart);
      stream_.UncheckedConsumeInternal();
    }

    ~BlockGuard() {
      if (stream_.PeekInternal().IsEOF())
        return;

      // We are already in a block, so start at one nesting level.
      stream_.UncheckedConsumeComponentValue(1);
    }

   private:
    CSSParserTokenStream& stream_;
  };

  explicit CSSParserTokenStream(CSSTokenizer& tokenizer)
      : tokenizer_(tokenizer), next_index_(0) {
    // TODO(shend): Add DCHECK that there are no tokens in tokenizer once
    // observers work with streams.
  }

  CSSParserTokenStream(CSSParserTokenStream&&) = default;

  const CSSParserToken& Peek() const {
    const CSSParserToken& token = PeekInternal();
    if (token.GetBlockType() == CSSParserToken::kBlockEnd)
      return g_static_eof_token;
    return token;
  }

  const CSSParserToken& UncheckedPeek() const {
    DCHECK_LT(next_index_, tokenizer_.tokens_.size());
    return tokenizer_.tokens_[next_index_];
  }

  const CSSParserToken& Consume() {
    const CSSParserToken& token = Peek();
    DCHECK_NE(token.GetBlockType(), CSSParserToken::kBlockStart);

    if (!token.IsEOF())
      next_index_++;

    DCHECK_LE(next_index_, tokenizer_.tokens_.size());
    return token;
  }

  const CSSParserToken& UncheckedConsume() {
    const CSSParserToken& token = UncheckedConsumeInternal();
    DCHECK_NE(token.GetBlockType(), CSSParserToken::kBlockStart);
    return token;
  }

  bool AtEnd() const { return Peek().IsEOF(); }

  // Range represents all tokens from current position to EOF.
  // Eagerly consumes all the remaining input.
  // TODO(shend): Remove this method once we switch over to using streams
  // completely.
  CSSParserTokenRange MakeRangeToEOF() {
    return tokenizer_.TokenRange().MakeSubRange(
        tokenizer_.tokens_.begin() + next_index_, tokenizer_.tokens_.end());
  }

  // Range represents all tokens that were consumed between begin and end.
  CSSParserTokenRange MakeSubRange(Iterator begin, Iterator end) {
    DCHECK_LE(begin.index_, tokenizer_.tokens_.size());
    DCHECK_LE(end.index_, tokenizer_.tokens_.size());
    DCHECK_LE(begin.index_, end.index_);
    const auto tokens_begin = tokenizer_.tokens_.begin();
    return CSSParserTokenRange(tokenizer_.tokens_)
        .MakeSubRange(tokens_begin + begin.index_, tokens_begin + end.index_);
  }

  Iterator Position() const {
    DCHECK_LE(next_index_, tokenizer_.tokens_.size());
    return Iterator(next_index_);
  }

  void ConsumeWhitespace();
  const CSSParserToken& ConsumeIncludingWhitespace();
  void UncheckedConsumeComponentValue(unsigned nesting_level = 0);

 private:
  const CSSParserToken& UncheckedConsumeInternal();
  const CSSParserToken& PeekInternal() const;

  CSSTokenizer& tokenizer_;
  size_t next_index_;  // Index of next token to be consumed.
};

}  // namespace blink

#endif  // CSSParserTokenStream_h
