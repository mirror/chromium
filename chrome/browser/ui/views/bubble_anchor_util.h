// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_BUBBLE_ANCHOR_UTIL_H_
#define CHROME_BROWSER_UI_VIEWS_BUBBLE_ANCHOR_UTIL_H_

#include "ui/gfx/geometry/point.h"

class Browser;

namespace views {
class View;
}

namespace bubble_anchor_util {

// These functions return the View or Point appropriate for anchoring a bubble
// to |browser|'s Page Info icon. These have separate implementations for Views-
// and Cocoa-based browsers. The anchor view, if non-null, takes precedence over
// the anchor point. The anchor point is in screen coordinates.

views::View* GetPageInfoAnchorView(Browser* browser);
gfx::Point GetPageInfoAnchorPoint(Browser* browser);

}  // namespace bubble_anchor_util

#endif  // CHROME_BROWSER_UI_VIEWS_BUBBLE_ANCHOR_UTIL_H_
