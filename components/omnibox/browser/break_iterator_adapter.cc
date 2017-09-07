// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/break_iterator_adapter.h"

#include "base/feature_list.h"
#include "components/omnibox/browser/omnibox_field_trial.h"

namespace {
const base::char16 underscore_ = '_';
}  // namespace

using base::i18n::BreakIterator;

BreakIteratorAdapter::BreakIteratorAdapter(const base::StringPiece16& str,
                                           BreakIterator::BreakType break_type)
    : BreakIterator(str, break_type),
      prev_(0),
      pos_(0),
      field_trial_enabled_(base::FeatureList::IsEnabled(
          omnibox::kExperimentUseUnderscoreAsPunctuation)) {}

BreakIteratorAdapter::~BreakIteratorAdapter() {}

bool BreakIteratorAdapter::HasWords() const {
  return pos_ != underscore_word_.size();
}

void BreakIteratorAdapter::AdvancePosition() {
  std::size_t found = underscore_word_.find(underscore_, pos_);
  prev_ = pos_;
  if (found == base::StringPiece16::npos) {
    pos_ = underscore_word_.size();
    return;
  }
  // If underscore found on current position, index moves to next char.
  if (pos_ == found)
    pos_ += 1;
  else
    pos_ = found;
}

bool BreakIteratorAdapter::Advance() {
  if (!field_trial_enabled_)
    return BreakIterator::Advance();
  if (HasWords()) {
    AdvancePosition();
    return true;
  }
  if (BreakIterator::Advance()) {
    prev_ = pos_ = 0;
    underscore_word_.clear();
    if (!IsWord())
      return true;
    base::StringPiece16 word = BreakIterator::GetStringPiece();
    std::size_t found = word.find(underscore_);
    if (found != base::StringPiece16::npos) {
      underscore_word_ = word;
      AdvancePosition();
    }
    return true;
  }
  return false;
}

bool BreakIteratorAdapter::IsWord() const {
  if (!field_trial_enabled_)
    return BreakIterator::IsWord();

  if (HasWords()) {
    base::string16 word = GetString();
    if (!word.empty())
      return word[0] != underscore_;
  }
  return BreakIterator::IsWord();
}

base::string16 BreakIteratorAdapter::GetString() const {
  if (!field_trial_enabled_)
    return BreakIterator::GetString();
  return GetStringPiece().as_string();
}

base::StringPiece16 BreakIteratorAdapter::GetStringPiece() const {
  if (!field_trial_enabled_)
    return BreakIterator::GetStringPiece();
  if (!underscore_word_.empty())
    return underscore_word_.substr(prev_, pos_ - prev_);
  return BreakIterator::GetStringPiece();
}

size_t BreakIteratorAdapter::prev() const {
  return BreakIterator::prev() + prev_;
}

size_t BreakIteratorAdapter::pos() const {
  return BreakIterator::pos() + pos_;
}
