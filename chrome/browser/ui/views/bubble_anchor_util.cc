// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/bubble_anchor_util.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "ui/base/ui_features.h"

#if !defined(OS_MACOSX) || BUILDFLAG(MAC_VIEWS_BROWSER)
views::View* BubbleAnchorUtil::GetAnchorView(Browser* browser,
                                             Location location) {
  if (location == Location::PAGE_INFO) {
    BrowserView* browser_view = static_cast<BrowserView*>(browser->window());
    return browser_view->toolbar()
        ->location_bar()
        ->GetSecurityBubbleAnchorView();
  }
  NOTREACHED();
  return nullptr;
}

gfx::Point BubbleAnchorUtil::GetAnchorPoint(Browser* browser,
                                            Location location) {
  return gfx::Point();
}
#endif
