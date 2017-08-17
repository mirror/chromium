// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/break_iterator_adapter.h"

#include "base/strings/string_split.h"

using base::i18n::BreakIterator;

BreakIteratorAdapter::BreakIteratorAdapter(const base::StringPiece16& str,
                                           BreakIterator::BreakType break_type)
    : BreakIterator(str, break_type) {}

BreakIteratorAdapter::~BreakIteratorAdapter() {}

bool BreakIteratorAdapter::Advance() {
  if (!offsets_.empty()) {
    UpdatePosition();
    return true;
  }

  if (!BreakIterator::Advance())
    return false;

  if (IsWord() && HasUnderscore()) {
    ResetPosition();
    UpdatePosition();
  } else {
    has_underscore_ = false;
  }
  return true;
}

bool BreakIteratorAdapter::HasUnderscore() {
  base::string16 word = BreakIterator::GetString();
  std::size_t found = word.find(underscore_);
  if (found == std::string::npos)
    return false;
  return true;
}

void BreakIteratorAdapter::ResetPosition() {
  offsets_.clear();
  prev_ = 0;
  pos_ = 0;
  has_underscore_ = true;

  string_ = BreakIterator::GetString();
  std::size_t found = std::string::npos;
  std::size_t offset = 0;

  while ((found = string_.find(underscore_, offset)) != std::string::npos) {
    offsets_.push_back(found);
    offset = found + 1;
  }

  if (offsets_.front() != 0)
    offsets_.insert(offsets_.begin(), std::string::npos);

  std::reverse(offsets_.begin(), offsets_.end());
}

void BreakIteratorAdapter::UpdatePosition() {
  if (offsets_.back() == std::string::npos)
    prev_ = 0;
  else
    prev_ = offsets_.back() + 1;
  offsets_.pop_back();
  if (offsets_.size())
    pos_ = offsets_.back();
  else
    pos_ = string_.size();
}

base::string16 BreakIteratorAdapter::GetString() const {
  if (has_underscore_)
    return string_.substr(prev_, pos_ - prev_);
  return BreakIterator::GetString();
}

size_t BreakIteratorAdapter::prev() const {
  if (has_underscore_)
    return BreakIterator::prev() + prev_;
  return BreakIterator::prev();
}

size_t BreakIteratorAdapter::pos() const {
  if (has_underscore_)
    return BreakIterator::pos() + pos_;
  return BreakIterator::pos();
}
