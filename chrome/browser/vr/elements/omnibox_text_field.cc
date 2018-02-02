// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/omnibox_text_field.h"

namespace vr {

OmniboxTextField::OmniboxTextField(
    float font_height_meters,
    OnInputEditedCallback input_edit_callback,
    base::RepeatingCallback<void(const AutocompleteRequest&)>
        autocomplete_start_callback,
    base::RepeatingCallback<void()> autocomplete_stop_callback)
    : TextInput(font_height_meters, input_edit_callback),
      autocomplete_start_callback_(autocomplete_start_callback),
      autocomplete_stop_callback_(autocomplete_stop_callback) {}

OmniboxTextField::~OmniboxTextField() {}

void OmniboxTextField::SetEnabled(bool enabled) {
  if (!enabled)
    autocomplete_stop_callback_.Run();
}

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
  request.prevent_inline_autocomplete = false;

  // If we have a non-leading cursor position, disable autocomplete. Note that
  // the autocomplete request does take in cursor position as a input.
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

  autocomplete_start_callback_.Run(request);
}

}  // namespace vr
