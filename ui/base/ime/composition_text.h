// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_COMPOSITION_TEXT_H_
#define UI_BASE_IME_COMPOSITION_TEXT_H_

#include <stddef.h>

#include "base/strings/string16.h"
#include "ui/base/ime/text_composition_data.h"
#include "ui/base/ime/ui_base_ime_export.h"
#include "ui/gfx/range/range.h"

namespace ui {

// A struct represents the status of an ongoing composition text.
struct UI_BASE_IME_EXPORT CompositionText {
  CompositionText();
  CompositionText(const CompositionText& other);
  ~CompositionText();

  bool operator==(const CompositionText& rhs) const {
    return this->text == rhs.text && this->selection == rhs.selection &&
           this->text_composition_data == rhs.text_composition_data;
  }

  bool operator!=(const CompositionText& rhs) const {
    return !(*this == rhs);
  }

  void Clear();

  void CopyFrom(const CompositionText& obj);

  // Content of the composition text.
  base::string16 text;

  // Other information, such as composition underlines.
  TextCompositionData text_composition_data;

  // Selection range in the composition text. It represents the caret position
  // if the range length is zero. Usually it's used for representing the target
  // clause (on Windows). Gtk doesn't have such concept, so background color is
  // usually used instead.
  gfx::Range selection;
};

}  // namespace ui

#endif  // UI_BASE_IME_COMPOSITION_TEXT_H_
