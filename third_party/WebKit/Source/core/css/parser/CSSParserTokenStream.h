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
      : tokenizer_(tokenizer),
        current_index_(0),
        offset_(tokenizer.input_.Offset()) {}

  CSSParserToken Peek() const {  // TODO: return by ref.
    if (current_index_ == tokenizer_.tokens_.size()) {
      if (tokenizer_.TokenizeSingle().GetType() ==
          CSSParserTokenType::kEOFToken)
        return CSSParserToken(CSSParserTokenType::kEOFToken);
    }
    DCHECK_EQ(current_index_, tokenizer_.tokens_.size());
    return tokenizer_.tokens_.back();
  }

  CSSParserToken Consume() {  // TODO: return by ref.
    CSSParserToken token = Peek();
    if (!token.IsEOF()) {
      current_index_++;
      offset_ = tokenizer_.input_.Offset();
    }

    DCHECK_EQ(current_index_, tokenizer_.tokens_.size());
    return token;
  }

  bool AtEnd() const { return Peek() == CSSParserTokenType::kEOFToken; }

  CSSParserToken ConsumeIncludingWhitespace() {
    CSSParserToken result = Consume();
    ConsumeWhitespace();
    return result;
  }

  void ConsumeWhitespace() {
    while (Peek().GetType() == kWhitespaceToken)
      ++current_index_;
  }

  size_t CurrentIndex() const { return current_index_; }

  // The returned range doesn't include the brackets
  CSSParserTokenRange ConsumeBlock() {
    DCHECK_EQ(Peek().GetBlockType(), CSSParserToken::kBlockStart);
    const size_t start = CurrentIndex() + 1;
    unsigned nesting_level = 0;
    do {
      const CSSParserToken& token = Consume();
      if (token.GetBlockType() == CSSParserToken::kBlockStart)
        nesting_level++;
      else if (token.GetBlockType() == CSSParserToken::kBlockEnd)
        nesting_level--;
    } while (nesting_level && !AtEnd());

    return TokenRangeFrom(start);
  }

  void ConsumeComponentValue() {
    unsigned nesting_level = 0;
    do {
      const CSSParserToken& token = Consume();
      if (token.GetBlockType() == CSSParserToken::kBlockStart)
        nesting_level++;
      else if (token.GetBlockType() == CSSParserToken::kBlockEnd)
        nesting_level--;
    } while (nesting_level && !AtEnd());
  }

  /*
  String Serialize() const;

  const CSSParserToken* begin() const { return first_; }
  const CSSParserToken* end() const { return last_; }

  static void InitStaticEOFToken();*/

  CSSParserTokenRange TokenRangeToEnd() const {
    return CSSParserTokenRange(tokenizer_.tokens_)
        .MakeSubRange(tokenizer_.tokens_.begin() + current_index_,
                      tokenizer_.tokens_.end());
  }

  CSSParserTokenRange TokenRangeFrom(size_t start_index) const {
    return TokenRangeBetween(start_index, current_index_);
  }

  CSSParserTokenRange TokenRangeBetween(size_t start_index,
                                        size_t end_index) const {
    DCHECK_LT(start_index, current_index_);
    const auto begin = tokenizer_.tokens_.begin();
    return CSSParserTokenRange(tokenizer_.tokens_)
        .MakeSubRange(begin + start_index, begin + end_index);
  }

  CSSParserTokenStream MakeBlockStream() { return *this; }

  size_t Offset() const { return offset_; }

 private:
  /*
  void EnsureTokenizedNext() {
    if (current_index_ == tokenizer_.tokens_.size()) {
      if (tokenizer_.tokens_.TokenizeSingle().GetType() ==
  CSSParserTokenType::kEOFToken); return staticEOFToken; // magic
    }
    return true;
  }*/

  CSSTokenizer& tokenizer_;
  size_t current_index_;
  size_t offset_;
};

}  // namespace blink

#endif  // CSSParserTokenStream_h
