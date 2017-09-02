// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/browser_list/browser_web_state_list_delegate.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BrowserWebStateListDelegate::BrowserWebStateListDelegate(Browser* browser)
    : browser_(browser) {
  DCHECK(browser_);
}

BrowserWebStateListDelegate::~BrowserWebStateListDelegate() = default;

#pragma mark - Public

void BrowserWebStateListDelegate::AddWebStateHelper(
    std::unique_ptr<BrowserWebStateHelper> helper) {
  DCHECK(helper);
  WebStateList& web_state_list = browser_->web_state_list();
  for (int index = 0; index < web_state_list.count(); ++index) {
    helper->OnWebStateAdded(web_state_list.GetWebStateAt(index));
  }
  web_state_helpers_.push_back(std::move(helper));
}

#pragma mark - WebStateListDelegate

void BrowserWebStateListDelegate::WillAddWebState(web::WebState* web_state) {
  for (auto& helper : web_state_helpers_) {
    helper->OnWebStateAdded(web_state);
  }
}

void BrowserWebStateListDelegate::WebStateDetached(web::WebState* web_state) {
  for (auto& helper : web_state_helpers_) {
    helper->OnWebStateDetached(web_state);
  }
}
