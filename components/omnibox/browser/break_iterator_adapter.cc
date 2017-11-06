// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/break_iterator_adapter.h"

#include "base/feature_list.h"

namespace {
constexpr base::char16 underscore_ = '_';
}  // namespace

using base::i18n::BreakIterator;

BreakIteratorAdapter::BreakIteratorAdapter(const base::StringPiece16& str,
                                           BreakIterator::BreakType break_type)
    : BreakIterator(str, break_type), prev_(0), pos_(0) {
  DCHECK(break_type == BreakIterator::BREAK_WORD);
}

BreakIteratorAdapter::~BreakIteratorAdapter() {}

bool BreakIteratorAdapter::Advance() {
  if (HasUnderscoreWords()) {
    AdvanceInUnderscoreWords();
    return true;
  }
  if (BreakIterator::Advance()) {
    prev_ = 0;
    pos_ = 0;
    underscore_word_.clear();
    if (!IsWord())
      return true;
    base::StringPiece16 word = BreakIterator::GetStringPiece();
    if (word.find(underscore_) != base::StringPiece16::npos) {
      underscore_word_ = word;
      AdvanceInUnderscoreWords();
    }
    return true;
  }
  return false;
}

bool BreakIteratorAdapter::IsWord() const {
  if (HasUnderscoreWords()) {
    base::string16 word = GetString();
    if (!word.empty())
      return word[0] != underscore_;
  }
  return BreakIterator::IsWord();
}

base::StringPiece16 BreakIteratorAdapter::GetStringPiece() const {
  if (!underscore_word_.empty())
    return underscore_word_.substr(prev_, pos_ - prev_);
  return BreakIterator::GetStringPiece();
}

base::string16 BreakIteratorAdapter::GetString() const {
  return GetStringPiece().as_string();
}

size_t BreakIteratorAdapter::prev() const {
  return BreakIterator::prev() + prev_;
}

size_t BreakIteratorAdapter::pos() const {
  return BreakIterator::pos() + pos_;
}

bool BreakIteratorAdapter::HasUnderscoreWords() const {
  return pos_ != underscore_word_.size();
}

void BreakIteratorAdapter::AdvanceInUnderscoreWords() {
  std::size_t next_pos = underscore_word_.find(underscore_, pos_);
  prev_ = pos_;
  if (next_pos == base::StringPiece16::npos) {
    pos_ = underscore_word_.size();
    return;
  }
  // If underscore found on current position, index moves to next char.
  if (pos_ == next_pos)
    pos_ += 1;
  else
    pos_ = next_pos;
}
