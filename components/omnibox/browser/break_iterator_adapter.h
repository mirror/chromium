// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_BREAK_ITERATOR_ADAPTER_H_
#define COMPONENTS_OMNIBOX_BROWSER_BREAK_ITERATOR_ADAPTER_H_

#include <vector>

#include "base/i18n/break_iterator.h"
#include "base/strings/utf_string_conversions.h"

// Adapter class for base::i18n::BreakIterator to allow breaking words at
// underscores.
class BreakIteratorAdapter : public base::i18n::BreakIterator {
 public:
  BreakIteratorAdapter(const base::StringPiece16& str,
                       base::i18n::BreakIterator::BreakType break_type);

  ~BreakIteratorAdapter();

  bool Advance();
  base::string16 GetString() const;
  size_t prev() const;
  size_t pos() const;

 private:
  bool HasUnderscore();
  void ResetPosition();
  void UpdatePosition();

  const base::string16 underscore_{base::ASCIIToUTF16("_")};

  base::string16 string_;
  size_t prev_, pos_;
  // Offsets of underscores in |string_|.
  std::vector<size_t> offsets_;
  bool has_underscore_;

  DISALLOW_COPY_AND_ASSIGN(BreakIteratorAdapter);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_BREAK_ITERATOR_ADAPTER_H_
