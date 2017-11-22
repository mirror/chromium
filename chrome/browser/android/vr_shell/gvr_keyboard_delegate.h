// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_SHELL_GVR_KEYBOARD_DELEGATE_H_
#define CHROME_BROWSER_ANDROID_VR_SHELL_GVR_KEYBOARD_DELEGATE_H_

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/android/vr_shell/vr_controller.h"
#include "chrome/browser/vr/keyboard_delegate.h"
#include "chrome/browser/vr/keyboard_ui_interface.h"
#include "chrome/browser/vr/model/text_input_info.h"
#include "third_party/gvr-android-keyboard/src/libraries/headers/vr/gvr/capi/include/gvr_keyboard.h"

namespace vr {
struct CameraModel;
}

namespace vr_shell {

class GvrKeyboardDelegate : public vr::KeyboardDelegate {
 public:
  typedef base::Callback<void(const vr::TextInputInfo&)> InputEditCallback;
  GvrKeyboardDelegate(VrController* controller, InputEditCallback callback);
  ~GvrKeyboardDelegate() override;

  typedef int32_t EventType;
  typedef base::Callback<void(EventType)> OnEventCallback;

  void EditInput(const vr::TextInputInfo& info);

  void OnBeginFrame() override;
  void ShowKeyboard() override;
  void HideKeyboard() override;
  void SetTransform(gfx::Transform transform) override;
  bool HitTest(const gfx::Point3F& ray_origin,
               const gfx::Point3F& ray_target,
               gfx::Point3F* hit_position) override;
  void Draw(const vr::CameraModel& model) override;

 private:
  void OnGvrKeyboardEvent(EventType);
  void OnInputEdited();

  gvr_keyboard_context* gvr_keyboard_ = nullptr;
  VrController* controller_;
  InputEditCallback input_edit_callback_;
  OnEventCallback keyboard_event_callback_;

  DISALLOW_COPY_AND_ASSIGN(GvrKeyboardDelegate);
};

}  // namespace vr_shell

#endif  // CHROME_BROWSER_ANDROID_VR_SHELL_GVR_KEYBOARD_DELEGATE_H_
