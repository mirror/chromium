// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_TEXT_INPUT_H_
#define CHROME_BROWSER_VR_ELEMENTS_TEXT_INPUT_H_

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "third_party/skia/include/core/SkColor.h"

namespace vr {

class Rect;
class Text;

class TextInput : public UiElement {
 public:
  TextInput(int maximum_width_pixels, float font_height_meters);
  ~TextInput() override;

  void SetText(const base::string16& text);
  void SetHintText(const base::string16& text);
  void SetCursorPosition(int position);
  void SetTextColor(SkColor color);
  void SetCursorColor(SkColor color);

  typedef base::Callback<void(const base::string16& text)> TextInputCallback;
  void SetTextChangedCallback(const TextInputCallback& callback);

  bool OnBeginFrame(const base::TimeTicks& time,
                    const gfx::Vector3dF& look_at) final;
  void OnSetSize(gfx::SizeF size) final;
  void OnSetName() final;

 private:
  void LayOutChildren() final;
  bool SetCursorBlinkState(const base::TimeTicks& time);

  base::string16 text_;
  TextInputCallback text_changed_callback_;
  bool cursor_visible_ = false;

  Text* hint_element_ = nullptr;
  Text* text_element_ = nullptr;
  Rect* cursor_element_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TextInput);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_TEXT_INPUT_H_
