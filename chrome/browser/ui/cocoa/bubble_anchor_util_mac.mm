// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/bubble_anchor_util.h"

#include "chrome/browser/ui/cocoa/bubble_anchor_helper.h"
#import "ui/gfx/mac/coordinate_conversion.h"

// This file contains the bubble_anchor_util implementation for
// BrowserWindowCocoa.

namespace bubble_anchor_util {

gfx::Point GetPageInfoAnchorPoint(Browser* browser) {
  return gfx::ScreenPointFromNSPoint(GetPageInfoAnchorPointForBrowser(
      browser, HasVisibleLocationBarForBrowser(browser)));
}

}  // namespace bubble_anchor_util
