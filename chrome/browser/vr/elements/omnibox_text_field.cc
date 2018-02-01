// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/omnibox_text_field.h"

#include <memory>

#include "base/bind.h"
#include "chrome/browser/vr/elements/rect.h"
#include "chrome/browser/vr/elements/text.h"
#include "chrome/browser/vr/elements/ui_texture.h"
#include "ui/gfx/geometry/rect.h"

namespace vr {

OmniboxTextField::OmniboxTextField(
    float font_height_meters,
    OnFocusChangedCallback focus_changed_callback,
    OnInputEditedCallback input_edit_callback,
    base::RepeatingCallback<void(const AutocompleteRequest&)>
        autocomplete_callback)
    : TextInput(font_height_meters,
                focus_changed_callback,
                input_edit_callback),
      autocomplete_callback_(autocomplete_callback)

{}

OmniboxTextField::~OmniboxTextField() {}

// TODO(cjgrant): Test moving cursor around without changing text.
// TODO(cjgrant): Moving cursor around without changing text.
// TODO(cjgrant): Sequence the interaction between new input and previous
//                suggestions.
// TODO(cjgrant): Multiple top matches may arrive in sequence.

void OmniboxTextField::SetAutocompletion(const Autocompletion& autocompletion) {
  if (autocompletion.suffix.empty())
    return;

  TextInputInfo current = text_info();
  base::string16 current_base = current.text.substr(0, current.selection_start);
  if (current_base != autocompletion.input)
    return;

  TextInputInfo info;
  info.text = current_base + autocompletion.suffix;
  info.selection_start = current_base.size();
  info.selection_end = info.text.size();
  UpdateInput(info);
}

void OmniboxTextField::OnUpdateInput(const TextInputInfo& info,
                                     const TextInputInfo& previous_info) {
  if (info.SelectionSize() > 0)
    return;

  AutocompleteRequest request;
  request.text = info.text;
  request.cursor_position = info.selection_end;

  // If we have a non-leading cursor position, disable autocomplete. Note that
  // the autocomplete request does take in cursor position as a input.
  request.prevent_inline_autocomplete = false;
  if (info.selection_end != static_cast<int>(info.text.size())) {
    request.prevent_inline_autocomplete = true;
  }

  // If the new unselected portion is not larger than the previous
  // unselected portion, disable autocomplete, as the user backspaced or removed
  // a selection.
  int old_base_size = previous_info.text.size() - previous_info.SelectionSize();
  int new_base_size = info.text.size() - info.SelectionSize();
  if (new_base_size <= old_base_size) {
    request.prevent_inline_autocomplete = true;
  }

  autocomplete_callback_.Run(request);
}

}  // namespace vr
