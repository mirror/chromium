// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/text_input_client.h"

namespace ui {

TextInputClient::~TextInputClient() {
}

void TextInputClient::GetTextInputClientInfo(
    GetTextInputClientInfoCallback callback) const {
  if (!callback || callback.is_null())
    return;

  gfx::Rect composition_head;
  if (HasCompositionText())
    GetCompositionCharacterBounds(0, &composition_head);

  // Pepper doesn't support composition bounds, so fall back to caret bounds to
  // avoid a bad user experience (the IME window moved to upper left corner).
  if (composition_head.IsEmpty())
    composition_head = GetCaretBounds();

  gfx::Range text_range;
  gfx::Range selection_range;
  base::string16 surrounding_text;

  if (!GetTextRange(&text_range) ||
      !GetTextFromRange(text_range, &surrounding_text) ||
      !GetSelectionRange(&selection_range)) {
    std::move(callback).Run(false, gfx::Range(), base::string16(), gfx::Range(),
                            composition_head);
  } else {
    std::move(callback).Run(true, text_range, surrounding_text, selection_range,
                            composition_head);
  }
}

}  // namespace ui
