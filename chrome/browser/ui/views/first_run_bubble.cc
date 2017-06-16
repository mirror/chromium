// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/first_run_bubble.h"

#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/first_run_bubble_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "ui/base/ui_features.h"

namespace {

views::WidgetDelegate* ShowBubbleImpl(Browser* browser,
                                      views::View* anchor_view,
                                      const gfx::Point& anchor_point,
                                      gfx::NativeWindow parent_window) {
  first_run::LogFirstRunMetric(first_run::FIRST_RUN_BUBBLE_SHOWN);
  FirstRunBubbleView* delegate = new FirstRunBubbleView(browser);
  delegate->UpdateAnchors(anchor_view, anchor_point, parent_window);
  views::BubbleDialogDelegateView::CreateBubble(delegate)->ShowInactive();
  return delegate;
}

}  // namespace

void FirstRunBubble::ShowBubble(Browser* browser) {
  ShowBubbleImpl(browser, GetAnchorView(browser), GetAnchorPoint(browser),
                 browser->window()->GetNativeWindow());
}

views::WidgetDelegate* FirstRunBubble::ShowBubbleForTest(
    views::View* anchor_view) {
  return ShowBubbleImpl(nullptr, anchor_view, gfx::Point(),
                        anchor_view->GetWidget()->GetNativeWindow());
}

#if !defined(OS_MACOSX) || BUILDFLAG(MAC_VIEWS_BROWSER)
views::View* FirstRunBubble::GetAnchorView(Browser* browser) {
  BrowserView* browser_view = static_cast<BrowserView*>(browser->window());
  return browser_view->toolbar()->location_bar()->GetSecurityBubbleAnchorView();
}

gfx::Point FirstRunBubble::GetAnchorPoint(Browser* browser) {
  return gfx::Point();
}
#endif
