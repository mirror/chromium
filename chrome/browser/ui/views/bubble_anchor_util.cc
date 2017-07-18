// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/bubble_anchor_util.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "ui/base/ui_features.h"

// This file contains the bubble_anchor_util implementation for a Views browser
// window (BrowserView).

namespace bubble_anchor_util {

views::View* GetPageInfoAnchorView(Browser* browser) {
  BrowserView* browser_view = static_cast<BrowserView*>(browser->window());
  return browser_view->toolbar()->location_bar()->GetSecurityBubbleAnchorView();
}

gfx::Point GetPageInfoAnchorPoint(Browser* browser) {
  return gfx::Point();
}

}  // namespace bubble_anchor_util
