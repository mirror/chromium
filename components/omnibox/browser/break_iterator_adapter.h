// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_BREAK_ITERATOR_ADAPTER_H_
#define COMPONENTS_OMNIBOX_BROWSER_BREAK_ITERATOR_ADAPTER_H_

#include <vector>

#include "base/i18n/break_iterator.h"

// Adapter class for base::i18n::BreakIterator to allow breaking words at
// underscores.
// If BreakIterator::GetStringPiece() has 'underscore', BreakIteratorAdapter
// works and deals with the underscore as punctuation in |underscore_word_|.
// Otherwise BreakIterator works.
class BreakIteratorAdapter : public base::i18n::BreakIterator {
 public:
  BreakIteratorAdapter(const base::StringPiece16& str,
                       base::i18n::BreakIterator::BreakType break_type);

  ~BreakIteratorAdapter() override;

  // Call BreakIterator::Advance(). And if
  // result of BreakIterator::GetStringPiece() has 'underscore', sets that word
  // to |underscore_word_|. (Otherwise it's empty)
  // If |underscore_word_| is set, next call for Advance() updates |prev_| and
  // |pos_| considering underscore.
  bool Advance() override;
  bool IsWord() const override;
  // Returns characters between |prev_| and |pos_| if |underscore_word_| is not
  // empty.
  base::StringPiece16 GetStringPiece() const override;
  base::string16 GetString() const override;
  size_t prev() const override;
  size_t pos() const override;

 private:
  // Check if |pos_| points the end of |underscore_word_|.
  // If returns false, BreakIterator works instead of BreakIteratorAdapter.
  bool HasUnderscoreWords() const;
  // Update |prev_| and |pos_| considering underscore.
  void AdvanceInUnderscoreWords();
  // |prev_| and |pos_| are indices to |underscore_word_|.
  size_t prev_, pos_;
  // Set if BreakIterator::GetStringPiece() contains 'underscore', otherwise
  // it's empty.
  base::StringPiece16 underscore_word_;

  DISALLOW_COPY_AND_ASSIGN(BreakIteratorAdapter);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_BREAK_ITERATOR_ADAPTER_H_
