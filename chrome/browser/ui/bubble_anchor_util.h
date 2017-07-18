// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BUBBLE_ANCHOR_UTIL_H_
#define CHROME_BROWSER_UI_BUBBLE_ANCHOR_UTIL_H_

#include "ui/gfx/geometry/point.h"

class Browser;

namespace bubble_anchor_util {

// Offset from the screen edge to show dialogs when there is no location bar.
// Don't center, since that could obscure a fullscreen bubble.
constexpr int kFullscreenLeftOffset = 40;

// Returns the Point appropriate for anchoring a bubble to |browser|'s Page Info
// icon, or an appropriate fallback when that is not visible. These have
// separate implementations for Views- and Cocoa-based browsers. The anchor
// point is in screen coordinates.
gfx::Point GetPageInfoAnchorPoint(Browser* browser);

}  // namespace bubble_anchor_util

#endif  // CHROME_BROWSER_UI_BUBBLE_ANCHOR_UTIL_H_
