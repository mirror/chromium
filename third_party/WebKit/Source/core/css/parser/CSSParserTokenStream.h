// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSParserTokenStream_h
#define CSSParserTokenStream_h

#include "CSSTokenizer.h"
#include "core/css/parser/CSSParserTokenRange.h"
#include "platform/wtf/Noncopyable.h"

namespace blink {

class CSSParserObserverWrapper;

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
      const CSSParserToken next = stream.ConsumeInternal();
      DCHECK_EQ(next.GetBlockType(), CSSParserToken::kBlockStart);
    }

    ~BlockGuard() {
      if (stream_.EnsureLookAhead() == LookAhead::kIsEOF)
        return;

      stream_.UncheckedSkipToEndOfBlock();
    }

   private:
    CSSParserTokenStream& stream_;
  };

  explicit CSSParserTokenStream(CSSTokenizer& tokenizer)
      : tokenizer_(tokenizer), next_index_(0) {
    DCHECK_EQ(tokenizer.CurrentSize(), 0U);
  }

  CSSParserTokenStream(CSSParserTokenStream&&) = default;

  // LookAhead::kIsEOF means the next token is EOF. Since we don't store EOF
  // tokens, tokenizer_.tokens_[next_index_] is valid iff LookAhead::kIsValid.
  enum class LookAhead { kIsValid, kIsEOF };
  LookAhead EnsureLookAhead() const {
    if (next_index_ == tokenizer_.CurrentSize()) {
      // Reached end of token buffer, but might not be end of input.
      if (tokenizer_.TokenizeSingle().IsEOF())
        return LookAhead::kIsEOF;
    }
    DCHECK(HasLookAhead());
    return LookAhead::kIsValid;
  }

  // Whether we have peeked at the next token or not.
  bool HasLookAhead() const {
    return next_index_ == tokenizer_.CurrentSize() - 1;
  }

  const CSSParserToken& Peek() const {
    const CSSParserToken& token = PeekInternal();
    if (token.GetBlockType() == CSSParserToken::kBlockEnd)
      return g_static_eof_token;
    return token;
  }

  const CSSParserToken& UncheckedPeek() const {
    const CSSParserToken& token = UncheckedPeekInternal();
    if (token.GetBlockType() == CSSParserToken::kBlockEnd)
      return g_static_eof_token;
    return token;
  }

  const CSSParserToken& Consume() {
    const CSSParserToken& token = ConsumeInternal();
    DCHECK_NE(token.GetBlockType(), CSSParserToken::kBlockStart);
    return token;
  }

  const CSSParserToken& UncheckedConsume() {
    const CSSParserToken& token = UncheckedConsumeInternal();
    DCHECK_NE(token.GetBlockType(), CSSParserToken::kBlockStart);
    DCHECK(!HasLookAhead());
    return token;
  }

  bool AtEnd() const { return Peek().IsEOF(); }

  // Range represents all tokens that were consumed between begin and end.
  CSSParserTokenRange MakeSubRange(Iterator begin, Iterator end) {
    DCHECK_LE(begin.index_, tokenizer_.CurrentSize());
    DCHECK_LE(end.index_, tokenizer_.CurrentSize());
    DCHECK_LE(begin.index_, end.index_);
    const auto tokens_begin = tokenizer_.tokens_.begin();
    return CSSParserTokenRange(tokenizer_.tokens_)
        .MakeSubRange(tokens_begin + begin.index_, tokens_begin + end.index_);
  }

  Iterator Position() const {
    DCHECK_LE(next_index_, tokenizer_.CurrentSize());
    return Iterator(next_index_);
  }

  // Get the index of the character in the original string to be consumed next.
  size_t Offset() const { return offset_; }

  // Get the index of the starting character of the look-ahead token.
  size_t LookAheadOffset() const {
    DCHECK(HasLookAhead() || AtEnd());
    return tokenizer_.PreviousOffset();
  }

  void ConsumeWhitespace();
  CSSParserToken ConsumeIncludingWhitespace();
  void UncheckedConsumeComponentValue(unsigned nesting_level = 0);
  void UncheckedConsumeComponentValueWithOffsets(CSSParserObserverWrapper&);
  // Either consumes a comment token and returns true, or peeks at the next
  // token and return false.
  bool ConsumeCommentOrNothing();

 private:
  const CSSParserToken& PeekInternal() const {
    if (EnsureLookAhead() == LookAhead::kIsEOF)
      return g_static_eof_token;
    return UncheckedPeekInternal();
  }

  const CSSParserToken& UncheckedPeekInternal() const {
    DCHECK(HasLookAhead());
    return tokenizer_.tokens_[next_index_];
  }

  const CSSParserToken& ConsumeInternal() {
    if (EnsureLookAhead() == LookAhead::kIsEOF)
      return g_static_eof_token;
    return UncheckedConsumeInternal();
  }

  const CSSParserToken& UncheckedConsumeInternal() {
    DCHECK(HasLookAhead());
    offset_ = tokenizer_.Offset();
    return tokenizer_.tokens_[next_index_++];
  }

  void UncheckedSkipToEndOfBlock() { UncheckedConsumeComponentValue(1); }

  CSSTokenizer& tokenizer_;
  size_t next_index_;  // Index of next token to be consumed.
  size_t offset_ = 0;
};

}  // namespace blink

#endif  // CSSParserTokenStream_h
