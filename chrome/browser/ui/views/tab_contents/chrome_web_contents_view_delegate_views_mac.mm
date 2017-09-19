// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/views/tab_contents/chrome_web_contents_view_delegate_views_mac.h"

#include "chrome/browser/ui/sad_tab_helper.h"
#include "chrome/browser/ui/views/sad_tab_view.h"
#include "chrome/browser/ui/views/tab_contents/chrome_web_contents_view_delegate_views.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/ui_features.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

ChromeWebContentsViewDelegateViewsMac::ChromeWebContentsViewDelegateViewsMac(
    content::WebContents* web_contents)
    : ChromeWebContentsViewDelegateMac(web_contents) {
  views_delegate_.reset(new ChromeWebContentsViewDelegateViews(web_contents));
}

ChromeWebContentsViewDelegateViewsMac::
    ~ChromeWebContentsViewDelegateViewsMac() {}

void ChromeWebContentsViewDelegateViewsMac::SizeChanged(const gfx::Size& size) {
  views_delegate_->SizeChanged(size);
}

bool ChromeWebContentsViewDelegateViewsMac::Focus() {
  return views_delegate_->Focus();
}

void ChromeWebContentsViewDelegateViewsMac::StoreFocus() {
  views_delegate_->StoreFocus();
}

void ChromeWebContentsViewDelegateViewsMac::RestoreFocus() {
  views_delegate_->RestoreFocus();
}

#if BUILDFLAG(MAC_VIEWS_BROWSER)
namespace chrome {

content::WebContentsViewDelegate* CreateWebContentsViewDelegate(
    content::WebContents* web_contents) {
  return new ChromeWebContentsViewDelegateViewsMac(web_contents);
}

}  // namespace chrome
#endif  // MAC_VIEWS_BROWSER
