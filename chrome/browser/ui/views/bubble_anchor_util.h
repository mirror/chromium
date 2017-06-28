// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_BUBBLE_ANCHOR_UTIL_H_
#define CHROME_BROWSER_UI_VIEWS_BUBBLE_ANCHOR_UTIL_H_

#include "ui/gfx/geometry/point.h"

class Browser;

namespace views {
class View;
}  // namespace views

class BubbleAnchorUtil {
 public:
  enum class Location {
    PAGE_INFO,
  };

  // These functions return the View or Point appropriate for anchoring a bubble
  // to |location|. These have separate implementations for Views- and
  // Cocoa-based browsers.
  static views::View* GetAnchorView(Browser* browser, Location location);
  static gfx::Point GetAnchorPoint(Browser* browser, Location location);
};

#endif  // CHROME_BROWSER_UI_VIEWS_BUBBLE_ANCHOR_UTIL_H_
