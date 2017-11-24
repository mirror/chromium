// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_SHELL_GVR_TEXT_INPUT_DELEGATE_H_
#define CHROME_BROWSER_ANDROID_VR_SHELL_GVR_TEXT_INPUT_DELEGATE_H_

#include "base/macros.h"
#include "chrome/browser/vr/elements/text_input.h"
#include "third_party/gvr-android-keyboard/src/libraries/headers/vr/gvr/capi/include/gvr_keyboard.h"

namespace vr {
struct TextInputInfo;
}

namespace vr_shell {

class GvrTextInputDelegate : public vr::TextInputDelegate {
 public:
  // RequestFocusCallback gets called when an element request's focus.
  typedef base::Callback<void(int)> RequestFocusCallback;
  // EditInputCallback gets called when the text input info changes for the
  // element being edited.
  typedef base::Callback<void(const vr::TextInputInfo&)> EditInputCallback;
  GvrTextInputDelegate(RequestFocusCallback request_focus_callback,
                       EditInputCallback edit_input_callback);
  ~GvrTextInputDelegate() override;

  void RequestFocus(int element_id) override;
  void EditInput(const vr::TextInputInfo& info) override;

 private:
  RequestFocusCallback request_focus_callback_;
  EditInputCallback edit_input_callback_;

  DISALLOW_COPY_AND_ASSIGN(GvrTextInputDelegate);
};

}  // namespace vr_shell

#endif  // CHROME_BROWSER_ANDROID_VR_SHELL_GVR_TEXT_INPUT_DELEGATE_H_
