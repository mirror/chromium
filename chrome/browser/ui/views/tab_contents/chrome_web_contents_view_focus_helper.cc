// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tab_contents/chrome_web_contents_view_focus_helper.h"

#include "chrome/browser/ui/sad_tab_helper.h"
#include "chrome/browser/ui/views/sad_tab_view.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view_tracker.h"
#include "ui/views/widget/widget.h"

ChromeWebContentsViewFocusHelper::ChromeWebContentsViewFocusHelper(
    content::WebContents* web_contents)
    : last_focused_view_tracker_(base::MakeUnique<views::ViewTracker>()),
      web_contents_(web_contents) {}

ChromeWebContentsViewFocusHelper::~ChromeWebContentsViewFocusHelper() {}

bool ChromeWebContentsViewFocusHelper::Focus() {
  SadTabHelper* sad_tab_helper = SadTabHelper::FromWebContents(web_contents_);
  if (sad_tab_helper) {
    SadTabView* sad_tab = static_cast<SadTabView*>(sad_tab_helper->sad_tab());
    if (sad_tab) {
      sad_tab->RequestFocus();
      return true;
    }
  }

  const web_modal::WebContentsModalDialogManager* manager =
      web_modal::WebContentsModalDialogManager::FromWebContents(web_contents_);
  if (manager && manager->IsDialogActive())
    manager->FocusTopmostDialog();

  return false;
}

void ChromeWebContentsViewFocusHelper::TakeFocus(bool reverse) {
  views::FocusManager* focus_manager = GetFocusManager();
  if (focus_manager)
    focus_manager->AdvanceFocus(reverse);
}

void ChromeWebContentsViewFocusHelper::StoreFocus() {
  last_focused_view_tracker_->Clear();
  if (GetFocusManager())
    last_focused_view_tracker_->SetView(GetFocusManager()->GetFocusedView());
}

void ChromeWebContentsViewFocusHelper::RestoreFocus() {
  views::View* last_focused_view = last_focused_view_tracker_->view();
  if (!last_focused_view) {
    SetInitialFocus();
  } else {
    if (last_focused_view->IsFocusable() &&
        GetFocusManager()->ContainsView(last_focused_view)) {
      last_focused_view->RequestFocus();
    } else {
      // The focused view may not belong to the same window hierarchy (e.g.
      // if the location bar was focused and the tab is dragged out), or it may
      // no longer be focusable (e.g. if the location bar was focused and then
      // we switched to fullscreen mode).  In that case we default to the
      // default focus.
      SetInitialFocus();
    }
    last_focused_view_tracker_->Clear();
  }
}

gfx::NativeView ChromeWebContentsViewFocusHelper::GetActiveNativeView() {
  return web_contents_->GetFullscreenRenderWidgetHostView() ?
      web_contents_->GetFullscreenRenderWidgetHostView()->GetNativeView() :
      web_contents_->GetNativeView();
}

views::Widget* ChromeWebContentsViewFocusHelper::GetTopLevelWidget() {
  return views::Widget::GetTopLevelWidgetForNativeView(GetActiveNativeView());
}

views::FocusManager* ChromeWebContentsViewFocusHelper::GetFocusManager() {
  views::Widget* toplevel_widget = GetTopLevelWidget();
  return toplevel_widget ? toplevel_widget->GetFocusManager() : NULL;
}

void ChromeWebContentsViewFocusHelper::SetInitialFocus() {
  if (web_contents_->FocusLocationBarByDefault()) {
    if (web_contents_->GetDelegate())
      web_contents_->GetDelegate()->SetFocusToLocationBar(false);
  } else {
    web_contents_->Focus();
  }
}
