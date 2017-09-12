// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSParserTokenStream_h
#define CSSParserTokenStream_h

#include "CSSTokenizer.h"
#include "core/css/parser/CSSParserTokenRange.h"
#include "platform/wtf/Noncopyable.h"

namespace blink {

class CSSParserRangeBuffer;
class CSSParserObserverWrapper;

// A streaming interface to CSSTokenizer that tokenizes on demand.
// Abstractly, the stream ends at either EOF or the beginning/end of a block.
// To consume a block, a BlockGuard must be created first to ensure that
// we finish consuming a block even if there was an error.
//
// Methods prefixed with "Unchecked" can only be called after calls to Peek(),
// EnsureLookAhead(), or AtEnd() with no subsequent modifications to the stream
// such as a consume.
class CORE_EXPORT CSSParserTokenStream {
  DISALLOW_NEW();
  WTF_MAKE_NONCOPYABLE(CSSParserTokenStream);

 public:
  // Instantiate this to start reading from a block. When the guard is out of
  // scope, the rest of the block is consumed.
  class BlockGuard {
   public:
    explicit BlockGuard(CSSParserTokenStream& stream) : stream_(stream) {
      const CSSParserToken next = stream.ConsumeInternal();
      DCHECK_EQ(next.GetBlockType(), CSSParserToken::kBlockStart);
    }

    ~BlockGuard() {
      stream_.EnsureLookAhead();
      stream_.UncheckedSkipToEndOfBlock();
    }

   private:
    CSSParserTokenStream& stream_;
  };

  explicit CSSParserTokenStream(CSSTokenizer& tokenizer)
      : buffer_(512), tokenizer_(tokenizer), next_(kEOFToken) {}

  CSSParserTokenStream(CSSParserTokenStream&&) = default;

  void EnsureLookAhead() {
    if (!HasLookAhead()) {
      has_look_ahead_ = true;
      next_ = tokenizer_.TokenizeSingle();
    }
  }

  bool HasLookAhead() const { return has_look_ahead_; }

  const CSSParserToken& Peek() {
    const CSSParserToken& token = PeekInternal();
    if (token.GetBlockType() == CSSParserToken::kBlockEnd)
      return g_static_eof_token;
    return token;
  }

  const CSSParserToken& UncheckedPeek() {
    const CSSParserToken& token = UncheckedPeekInternal();
    if (token.GetBlockType() == CSSParserToken::kBlockEnd)
      return g_static_eof_token;
    return token;
  }

  const CSSParserToken& Consume() {
    const CSSParserToken& token = ConsumeInternal();
    DCHECK_NE(token.GetBlockType(), CSSParserToken::kBlockStart);
    DCHECK(!HasLookAhead());
    return token;
  }

  const CSSParserToken& UncheckedConsume() {
    const CSSParserToken& token = UncheckedConsumeInternal();
    DCHECK_NE(token.GetBlockType(), CSSParserToken::kBlockStart);
    DCHECK(!HasLookAhead());
    return token;
  }

  bool AtEnd() { return Peek().IsEOF(); }

  // Get the index of the character in the original string to be consumed next.
  size_t Offset() const { return offset_; }

  // Get the index of the starting character of the look-ahead token.
  size_t LookAheadOffset() const {
    DCHECK(HasLookAhead());
    return tokenizer_.PreviousOffset();
  }

  void ConsumeWhitespace();
  CSSParserToken ConsumeIncludingWhitespace();
  void UncheckedConsumeComponentValue(unsigned nesting_level = 0);
  void UncheckedConsumeComponentValue(CSSParserRangeBuffer&);
  void UncheckedConsumeComponentValueWithOffsets(CSSParserObserverWrapper&);
  // Either consumes a comment token and returns true, or peeks at the next
  // token and return false.
  bool ConsumeCommentOrNothing();

 private:
  friend class CSSParserRangeBuffer;

  const CSSParserToken& PeekInternal() {
    EnsureLookAhead();
    return UncheckedPeekInternal();
  }

  const CSSParserToken& UncheckedPeekInternal() {
    DCHECK(HasLookAhead());
    return next_;
  }

  const CSSParserToken& ConsumeInternal() {
    EnsureLookAhead();
    return UncheckedConsumeInternal();
  }

  const CSSParserToken& UncheckedConsumeInternal() {
    DCHECK(HasLookAhead());
    has_look_ahead_ = false;
    offset_ = tokenizer_.Offset();
    return next_;
  }

  void UncheckedSkipToEndOfBlock() { UncheckedConsumeComponentValue(1); }

  CSSTokenizerBuffer buffer_;
  CSSTokenizer& tokenizer_;
  CSSParserToken next_;
  size_t offset_ = 0;
  bool has_look_ahead_ = false;
};

}  // namespace blink

#endif  // CSSParserTokenStream_h
