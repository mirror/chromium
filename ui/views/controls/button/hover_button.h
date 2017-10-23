// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_BUTTON_HOVER_BUTTON_H_
#define UI_VIEWS_CONTROLS_BUTTON_HOVER_BUTTON_H_

#include "ui/views/controls/button/label_button.h"

namespace views {

// A button that allows for setting a background color when hovered over.
class VIEWS_EXPORT HoverButton : public LabelButton {
 public:
  // Creates a single line hover button with no icon.
  HoverButton(ButtonListener* button_listener, const base::string16& text);

  // Creates a single line hover button with an icon.
  HoverButton(ButtonListener* button_listener,
              const gfx::ImageSkia& icon,
              const base::string16& text);

  // Creates a double line hover button with a custom view in place of the icon.
  // Passing an empty |subtitle| will adjust the |title| text to be vertically
  // centered to |icon_view|.
  HoverButton(ButtonListener* button_listener,
              views::View* icon_view,
              const base::string16& title,
              const base::string16& subtitle);

  ~HoverButton() override {}

  void SetSubtitleElideBehavior(gfx::ElideBehavior elide_behavior);

 private:
  // View:
  void OnFocus() override;
  void OnBlur() override;
  void OnNativeThemeChanged(const ui::NativeTheme* theme) override;

  // Button:
  void StateChanged(ButtonState old_state) override;

  void UpdateColors();

  Label* title_;
  Label* subtitle_;

  DISALLOW_COPY_AND_ASSIGN(HoverButton);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_BUTTON_HOVER_BUTTON_H_
