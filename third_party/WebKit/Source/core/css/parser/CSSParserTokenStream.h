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
// Methods prefixed with "Unchecked" can only be called after calls to Peek(),
// EnsureLookAhead(), or AtEnd() with no subsequent modifications to the stream
// such as a consume.
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
      stream_.EnsureLookAhead();
      stream_.UncheckedSkipToEndOfBlock();
    }

   private:
    CSSParserTokenStream& stream_;
  };

  explicit CSSParserTokenStream(CSSTokenizer& tokenizer)
      : tokenizer_(tokenizer), next_(kEOFToken), next_index_(0) {
    DCHECK_EQ(tokenizer.CurrentSize(), 0U);
  }

  CSSParserTokenStream(CSSParserTokenStream&&) = default;

  inline void EnsureLookAhead() {
    if (!has_look_ahead_) {
      has_look_ahead_ = true;
      next_ = tokenizer_.TokenizeSingle();
    }
  }

  // Forcibly read a lookahead token.
  inline void LookAhead() {
    DCHECK(!has_look_ahead_);
    next_ = tokenizer_.TokenizeSingle();
    has_look_ahead_ = true;
  }

  inline bool HasLookAhead() const { return has_look_ahead_; }

  inline const CSSParserToken& Peek() {
    EnsureLookAhead();
    return next_;
  }

  inline const CSSParserToken& UncheckedPeek() const {
    DCHECK(has_look_ahead_);
    return next_;
  }

  inline const CSSParserToken& Consume() {
    EnsureLookAhead();
    return UncheckedConsume();
  }

  inline const CSSParserToken& UncheckedConsume() {
    DCHECK(has_look_ahead_);
    has_look_ahead_ = false;
    next_index_++;
    offset_ = tokenizer_.Offset();
    return next_;
  }

  inline bool AtEnd() {
    EnsureLookAhead();
    return next_.IsEOF() || next_.GetBlockType() == CSSParserToken::kBlockEnd;
  }

  inline bool UncheckedAtEnd() const {
    DCHECK(has_look_ahead_);
    return next_.IsEOF() || next_.GetBlockType() == CSSParserToken::kBlockEnd;
  }

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
    DCHECK(HasLookAhead());
    return tokenizer_.PreviousOffset();
  }

  void ConsumeWhitespace();
  CSSParserToken ConsumeIncludingWhitespace();
  void UncheckedConsumeComponentValue(unsigned nesting_level = 0);
  void UncheckedConsumeComponentValueWithOffsets(CSSParserObserverWrapper&);
  // Either consumes a comment token and returns true, or peeks at the next
  // token and return false.
  bool ConsumeCommentOrNothing();

  template <CSSParserTokenType... Types>
  void UncheckedConsumeUntilLookAheadIs() {
    DCHECK(has_look_ahead_);

    unsigned nesting_level = tokenizer_.BlockNestingLevel();
    if (next_.GetBlockType() == CSSParserToken::kBlockStart)
      --nesting_level;

    const unsigned initial_nesting_level = nesting_level;
    while (true) {
      if (IsOneOf<Types..., kRightBraceToken>(next_.GetType()) &&
          initial_nesting_level == nesting_level)
        break;
      if (next_.IsEOF())
        break;

      next_index_++;
      offset_ = tokenizer_.Offset();
      nesting_level = tokenizer_.BlockNestingLevel();
      next_ = tokenizer_.TokenizeSingle();
    }
  }

 private:
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
    next_index_++;
    offset_ = tokenizer_.Offset();
    return next_;
  }

  void UncheckedSkipToEndOfBlock() {
    DCHECK(HasLookAhead());

    // Have to use internal consume/peek in here because they can read past
    // start/end of blocks
    unsigned nesting_level = 1;
    do {
      const CSSParserToken& token = UncheckedConsumeInternal();
      if (token.GetBlockType() == CSSParserToken::kBlockStart)
        nesting_level++;
      else if (token.GetBlockType() == CSSParserToken::kBlockEnd)
        nesting_level--;
    } while (nesting_level && !PeekInternal().IsEOF());
  }

  template <CSSParserTokenType Type,
            CSSParserTokenType Head,
            CSSParserTokenType... Tail>
  bool IsOneOf(CSSParserTokenType t) const {
    return t == Type || IsOneOf<Head, Tail...>(t);
  }

  template <CSSParserTokenType Type>
  bool IsOneOf(CSSParserTokenType t) const {
    return t == Type;
  }

  CSSTokenizer& tokenizer_;
  CSSParserToken next_;
  size_t next_index_;  // Index of next token to be consumed.
  size_t offset_ = 0;
  bool has_look_ahead_ = false;
};

}  // namespace blink

#endif  // CSSParserTokenStream_h
