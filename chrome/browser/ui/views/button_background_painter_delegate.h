// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_BUTTON_BACKGROUND_PAINTER_DELEGATE_H_
#define CHROME_BROWSER_UI_VIEWS_BUTTON_BACKGROUND_PAINTER_DELEGATE_H_

#include "build/buildflag.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/features.h"

#if !BUILDFLAG(ENABLE_NATIVE_WINDOW_NAV_BUTTONS)
#error "Include not allowed."
#endif

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
