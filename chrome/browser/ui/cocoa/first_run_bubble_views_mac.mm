// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/first_run_bubble.h"

#include "chrome/browser/ui/cocoa/bubble_anchor_helper.h"
#include "ui/gfx/mac/coordinate_conversion.h"

// static
views::View* FirstRunBubble::GetAnchorView(Browser* browser) {
  return nullptr;
}

// static
gfx::Point FirstRunBubble::GetAnchorPoint(Browser* browser) {
  return gfx::ScreenPointFromNSPoint(GetPermissionBubbleAnchorPointForBrowser(
      browser, HasVisibleLocationBarForBrowser(browser)));
}
