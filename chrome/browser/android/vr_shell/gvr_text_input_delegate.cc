// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr_shell/gvr_text_input_delegate.h"
#include "chrome/browser/vr/model/text_input_info.h"

namespace vr_shell {

GvrTextInputDelegate::GvrTextInputDelegate(
    RequestFocusCallback request_focus_callback,
    EditInputCallback edit_input_callback)
    : request_focus_callback_(request_focus_callback),
      edit_input_callback_(edit_input_callback) {}

GvrTextInputDelegate::~GvrTextInputDelegate() = default;

void GvrTextInputDelegate::RequestFocus(int element_id) {
  request_focus_callback_.Run(element_id);
}

void GvrTextInputDelegate::EditInput(const vr::TextInputInfo& info) {
  edit_input_callback_.Run(info);
}

}  // namespace vr_shell
