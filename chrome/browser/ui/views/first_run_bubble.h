// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FIRST_RUN_BUBBLE_H_
#define CHROME_BROWSER_UI_VIEWS_FIRST_RUN_BUBBLE_H_

#include "ui/gfx/geometry/point.h"
#include "ui/gfx/native_widget_types.h"

class Browser;

namespace views {
class WidgetDelegate;
class View;
}

class FirstRunBubble {
 public:
  static void ShowBubble(Browser* browser);
  static views::WidgetDelegate* ShowBubbleForTest(views::View* anchor_view);

  static views::View* GetAnchorView(Browser* browser);
  static gfx::Point GetAnchorPoint(Browser* browser);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FIRST_RUN_BUBBLE_H_
