// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_TEXT_INPUT_H_
#define CHROME_BROWSER_VR_ELEMENTS_TEXT_INPUT_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/vr/elements/rect.h"
#include "chrome/browser/vr/elements/ui_element.h"

namespace vr {

class KeyboardDelegate;
struct TextInputInfo;

class TextInputDelegate {
 public:
  virtual ~TextInputDelegate() {}
  virtual void EditInput(const TextInputInfo& info) = 0;
  virtual void RequestFocus(const UiElementName name) = 0;
};

// TODO(ymalik): This should be a TexturedElement.
class TextInput : public Rect {
 public:
  typedef base::Callback<void(bool)> OnFocusChangedCallback;
  typedef base::Callback<void(const TextInputInfo& info)> OnInputEditedCallback;
  TextInput(OnFocusChangedCallback focus_changed_callback,
            OnInputEditedCallback input_edit_callback);
  ~TextInput() override;

  void Initialize(SkiaSurfaceProvider* provider,
                  KeyboardDelegate* keyboard_delegate,
                  TextInputDelegate* text_input_delegate) final;
  bool editable() override;
  void OnButtonUp(const gfx::PointF& position) override;
  void OnFocusChanged(bool focused) override;
  void OnInputEdited(const TextInputInfo& info) override;

  void SetTextInputDelegate(TextInputDelegate* delegate);

  void EditInput(const TextInputInfo& info);

 private:
  void Render(UiElementRenderer* renderer,
              const CameraModel& camera_model) const final;

  TextInputDelegate* delegate_ = nullptr;
  OnFocusChangedCallback focus_changed_callback_;
  OnInputEditedCallback input_edit_callback_;
  bool focused_ = false;

  DISALLOW_COPY_AND_ASSIGN(TextInput);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_TEXT_INPUT_H_
