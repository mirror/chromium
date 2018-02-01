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

void OmniboxTextField::OnInputEdited(const TextInputInfo& info) {
  LOG(INFO) << "Omnibox got edit: " << info.text;

  // TODO - move this back to the callback/model approach.
  UpdateInput(info);
}

void OmniboxTextField::OnInputCommitted(const TextInputInfo& info) {
  LOG(INFO) << "Omnibox got commit: " << info.text;
  previous_input_ = info;
}

// TODO(cjgrant): Augment the suggestion struct to include the input that
// generated the suggestion, such that we can validate that the incoming
// suggestion actually fits the current text (and similarly, that new text never
// applies a previous incompatible match).

// TODO(cjgrant): Multiple top matches may arrive in sequence.
//
//
void OmniboxTextField::SetTopMatch(const OmniboxSuggestion& match) {
  top_match_ = std::make_unique<OmniboxSuggestion>(match);

  if (!match.inline_autocompletion.empty() &&
      previous_input_.SelectionSize() == 0 &&
      previous_input_.selection_end ==
          static_cast<int>(previous_input_.text.size())) {
    LOG(INFO) << "Omnibox got inline match: " << match.inline_autocompletion;
    auto info = previous_input_;
    info.text += match.inline_autocompletion;
    info.selection_start = previous_input_.text.size();
    info.selection_end = info.text.size();

    UpdateInput(info);
  }
}

void OmniboxTextField::ClearTopMatch() {
  LOG(INFO) << "Omnibox - clearing top match";
  top_match_.reset();
}

TextInputInfo OmniboxTextField::ProcessInput(const TextInputInfo& info) {
  if (info.SelectionSize() > 0) {
    LOG(INFO) << "Update has selection; not issuing new ACC request";
    return info;
  }

  AutocompleteRequest request;
  request.text = info.text;

  // Supply cursor position.
  if (info.SelectionSize() == 0) {
    request.cursor_position = info.selection_end;
  } else {
    request.cursor_position = base::string16::npos;
  }

  // If we have a non-leading cursor position, disable autocomplete. Note that
  // the autocomplete request does take in cursor position as a input.
  request.prevent_inline_autocomplete = false;
  if (info.selection_end != static_cast<int>(info.text.size())) {
    request.prevent_inline_autocomplete = true;
  }

  // If the new unselected portion is not larger than the previous
  // unselected portion, disable autocomplete, as the user backspaced or removed
  // a selection.
  int old_base_size =
      previous_input_.text.size() - previous_input_.SelectionSize();
  int new_base_size = info.text.size() - info.SelectionSize();
  if (new_base_size <= old_base_size) {
    request.prevent_inline_autocomplete = true;
  }

  autocomplete_callback_.Run(request);

  previous_input_ = info;
  return info;
}

}  // namespace vr
