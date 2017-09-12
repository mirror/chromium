// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_BREAK_ITERATOR_ADAPTER_H_
#define COMPONENTS_OMNIBOX_BROWSER_BREAK_ITERATOR_ADAPTER_H_

#include <vector>

#include "base/i18n/break_iterator.h"

// Adapter class for base::i18n::BreakIterator to allow breaking words at
// underscores.
class BreakIteratorAdapter : public base::i18n::BreakIterator {
 public:
  BreakIteratorAdapter(const base::StringPiece16& str,
                       base::i18n::BreakIterator::BreakType break_type);

  ~BreakIteratorAdapter() override;

  bool Advance() override;
  bool IsWord() const override;
  base::string16 GetString() const override;
  base::StringPiece16 GetStringPiece() const override;
  size_t prev() const override;
  size_t pos() const override;

 private:
  // Check if |pos_| reaches the end of |underscore_word_|.
  bool HasWords() const;
  // Move |prev_| and |pos_|.
  // |underscore_word_| contains underscores that shoud be treated as a
  // punctuation except valid words. So move indices based on underscore.
  void AdvancePosition();
  // |prev_| and |pos_| are indices of |underscore_word_|.
  size_t prev_, pos_;
  // After BreakIterator::Advance(), if BreakIterator::IsWord() is true and
  // BreakIterator::GetStringPiece() has 'underscore', |underscore_word_|
  // has that word.
  base::StringPiece16 underscore_word_;

  DISALLOW_COPY_AND_ASSIGN(BreakIteratorAdapter);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_BREAK_ITERATOR_ADAPTER_H_
