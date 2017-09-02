// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web_state_helper_data_source/browser_web_state_delegate_helper.h"

#import "ios/chrome/browser/web_state_helper_data_source/browser_web_state_delegate.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BrowserWebStateDelegateHelper::BrowserWebStateDelegateHelper(Browser* browser) {
  BrowserWebStateDelegate::CreateForBrowser(browser);
  web_state_delegate_ = BrowserWebStateDelegate::FromBrowser(browser);
}

BrowserWebStateDelegateHelper::~BrowserWebStateDelegateHelper() = default;

void BrowserWebStateDelegateHelper::OnWebStateAdded(web::WebState* web_state) {
  DCHECK(!web_state->GetDelegate());
  web_state->SetDelegate(web_state_delegate_);
}

void BrowserWebStateDelegateHelper::OnWebStateDetached(
    web::WebState* web_state) {
  DCHECK_EQ(web_state->GetDelegate(), web_state_delegate_);
  web_state->SetDelegate(web_state_delegate_);
}
