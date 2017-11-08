// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/views/tab_contents/chrome_web_contents_view_delegate_views_mac.h"

#include "chrome/browser/ui/sad_tab_helper.h"
#include "chrome/browser/ui/views/sad_tab_view.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/ui_features.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view_tracker.h"
#include "ui/views/widget/widget.h"

ChromeWebContentsViewDelegateViewsMac::ChromeWebContentsViewDelegateViewsMac(
    content::WebContents* web_contents)
    : ChromeWebContentsViewDelegateMac(web_contents),
      last_focused_view_tracker_(base::MakeUnique<views::ViewTracker>()) {}

ChromeWebContentsViewDelegateViewsMac ::
    ~ChromeWebContentsViewDelegateViewsMac() {}

void ChromeWebContentsViewDelegateViewsMac::StoreFocus() {
  last_focused_view_tracker_->Clear();
  if (GetFocusManager())
    last_focused_view_tracker_->SetView(GetFocusManager()->GetFocusedView());
}

bool ChromeWebContentsViewDelegateViewsMac::RestoreFocus() {
  views::View* last_focused_view = last_focused_view_tracker_->view();
  last_focused_view_tracker_->Clear();
  if (last_focused_view && last_focused_view->IsFocusable() &&
      GetFocusManager()->ContainsView(last_focused_view)) {
    last_focused_view->RequestFocus();
    return true;
  }
  return false;
}

void ChromeWebContentsViewDelegateViewsMac::ResetStoredFocus() {
  last_focused_view_tracker_->Clear();
}

bool ChromeWebContentsViewDelegateViewsMac::TakeFocus(bool reverse) {
  views::FocusManager* focus_manager = GetFocusManager();
  if (focus_manager) {
    focus_manager->AdvanceFocus(reverse);
    return true;
  }
  return false;
}

void ChromeWebContentsViewDelegateViewsMac::SizeChanged(const gfx::Size& size) {
  SadTabHelper* sad_tab_helper = SadTabHelper::FromWebContents(web_contents());
  if (!sad_tab_helper)
    return;
  SadTabView* sad_tab = static_cast<SadTabView*>(sad_tab_helper->sad_tab());
  if (sad_tab)
    sad_tab->GetWidget()->SetBounds(gfx::Rect(size));
}

gfx::NativeView ChromeWebContentsViewDelegateViewsMac::GetActiveNativeView() {
  return web_contents()->GetFullscreenRenderWidgetHostView()
             ? web_contents()
                   ->GetFullscreenRenderWidgetHostView()
                   ->GetNativeView()
             : web_contents()->GetNativeView();
}

views::Widget* ChromeWebContentsViewDelegateViewsMac::GetTopLevelWidget() {
  return views::Widget::GetTopLevelWidgetForNativeView(GetActiveNativeView());
}

views::FocusManager* ChromeWebContentsViewDelegateViewsMac::GetFocusManager() {
  views::Widget* toplevel_widget = GetTopLevelWidget();
  return toplevel_widget ? toplevel_widget->GetFocusManager() : nullptr;
}

#if BUILDFLAG(MAC_VIEWS_BROWSER)
namespace chrome {

content::WebContentsViewDelegate* CreateWebContentsViewDelegate(
    content::WebContents* web_contents) {
  return new ChromeWebContentsViewDelegateViewsMac(web_contents);
}

}  // namespace chrome
#endif  // MAC_VIEWS_BROWSER
