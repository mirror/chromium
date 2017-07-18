// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/bubble_anchor_util_views.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"

// This file contains the bubble_anchor_util implementation for a Views
// browser window (BrowserView).

namespace bubble_anchor_util {

views::View* GetPageInfoAnchorView(Browser* browser) {
  if (!browser_->SupportsWindowFeature(Browser::FEATURE_LOCATIONBAR))
    return nullptr;  // Fall back to GetAnchorPoint().

  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser_);
  return browser_view->GetLocationBarView()->GetSecurityBubbleAnchorView();
}

gfx::Point GetPageInfoAnchorPoint(Browser* browser) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser_);
  // Get position in view (taking RTL displays into account).
  int x_within_browser_view = browser_view->GetMirroredXInView(
      bubble_anchor_util::kFullscreenLeftOffset);
  // Get position in screen (taking browser view origin into account, which may
  // not be 0,0 if there are multiple displays).
  gfx::Point browser_view_origin = browser_view->GetBoundsInScreen().origin();
  return browser_view_origin + gfx::Vector2d(x_within_browser_view, 0);
}

}  // namespace bubble_anchor_util
