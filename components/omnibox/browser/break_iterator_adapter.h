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

  ~BreakIteratorAdapter();

  bool Advance() override;
  bool IsWord() const override;
  base::string16 GetString() const override;
  base::StringPiece16 GetStringPiece() const override;
  size_t prev() const override;
  size_t pos() const override;

 private:
  bool HasWords() const;
  void AdvancePosition();
  size_t prev_, pos_;
  base::StringPiece16 underscore_word_;

  bool field_trial_enabled_;

  DISALLOW_COPY_AND_ASSIGN(BreakIteratorAdapter);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_BREAK_ITERATOR_ADAPTER_H_
