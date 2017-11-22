// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/text_input.h"
#include "chrome/browser/vr/model/text_input_info.h"

#include "base/memory/ptr_util.h"

namespace vr {

TextInput::TextInput(OnFocusChangedCallback focus_changed_callback,
                     OnInputEditedCallback input_edit_callback)
    : focus_changed_callback_(focus_changed_callback),
      input_edit_callback_(input_edit_callback) {}

TextInput::~TextInput() = default;

void TextInput::Initialize(SkiaSurfaceProvider* provider,
                           KeyboardDelegate* keyboard_delegate,
                           TextInputDelegate* text_input_delegate) {
  LOG(ERROR) << "lolk SetTextInputDelegate";
  delegate_ = text_input_delegate;
}

void TextInput::SetTextInputDelegate(TextInputDelegate* delegate) {
  delegate_ = delegate;
}

void TextInput::EditInput(const TextInputInfo& info) {
  if (!delegate_ || !focused_)
    return;

  LOG(ERROR) << "lolk TextInput::EditInput";

  delegate_->EditInput(info);
}

bool TextInput::editable() {
  return true;
}

void TextInput::OnButtonUp(const gfx::PointF& position) {
  if (!delegate_)
    return;

  delegate_->RequestFocus(name());
}

void TextInput::OnFocusChanged(bool focused) {
  LOG(ERROR) << "lolk TextInput::OnFocusChanged: " << focused;
  focused_ = focused;
  focus_changed_callback_.Run(focused);
}

void TextInput::OnInputEdited(const TextInputInfo& info) {
  LOG(ERROR) << "lolk TextInput::OnInputEdited: " << info.DebugString();
  input_edit_callback_.Run(info);
}

void TextInput::Render(UiElementRenderer* renderer,
                       const CameraModel& camera_model) const {
  Rect::Render(renderer, camera_model);
}

}  // namespace vr
