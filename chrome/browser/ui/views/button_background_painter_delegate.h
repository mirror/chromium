// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_BUTTON_BACKGROUND_PAINTER_DELEGATE_H_
#define CHROME_BROWSER_UI_VIEWS_BUTTON_BACKGROUND_PAINTER_DELEGATE_H_

#include "ui/views/controls/button/button.h"

namespace views {

class ButtonBackgroundPainterDelegate {
 public:
  virtual ~ButtonBackgroundPainterDelegate() {}

  // Provides the state used for painting the button.
  virtual Button::ButtonState CalculateButtonState() const = 0;

  // Gets the toplevel widget for the button.
  virtual const views::Widget* GetWidget() const = 0;
};

}  // namespace views

#endif  // CHROME_BROWSER_UI_VIEWS_BUTTON_BACKGROUND_PAINTER_DELEGATE_H_
