// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/web/web_state_delegate/web_state_delegate_service_impl.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/browser_list/browser_list.h"
#import "ios/clean/chrome/browser/web/web_state_delegate/web_state_delegate_impl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

WebStateDelegateServiceImpl::WebStateDelegateServiceImpl(
    BrowserList* browser_list)
    : browser_list_(browser_list), attached_(false) {
  DCHECK(browser_list_);
}

WebStateDelegateServiceImpl::~WebStateDelegateServiceImpl() {}

#pragma mark - BrowserListObserver

void WebStateDelegateServiceImpl::OnBrowserCreated(BrowserList* browser_list,
                                                   Browser* browser) {
  DCHECK(attached_);
  AttachWebStateDelegate(browser);
}

void WebStateDelegateServiceImpl::OnBrowserRemoved(BrowserList* browser_list,
                                                   Browser* browser) {
  DCHECK(attached_);
  DetachWebStateDelegate(browser);
}

#pragma mark - KeyedService

void WebStateDelegateServiceImpl::Shutdown() {
  if (attached_)
    DetachWebStateDelegates();
}

#pragma mark - WebStateDelegateService

void WebStateDelegateServiceImpl::AttachWebStateDelegates() {
  if (attached_)
    return;
  for (int index = 0; index < browser_list_->count(); ++index) {
    AttachWebStateDelegate(browser_list_->GetBrowserAtIndex(index));
  }
  browser_list_->AddObserver(this);
  attached_ = true;
}

void WebStateDelegateServiceImpl::DetachWebStateDelegates() {
  if (!attached_)
    return;
  for (int index = 0; index < browser_list_->count(); ++index) {
    DetachWebStateDelegate(browser_list_->GetBrowserAtIndex(index));
  }
  browser_list_->RemoveObserver(this);
  attached_ = false;
}

#pragma mark - Private

void WebStateDelegateServiceImpl::AttachWebStateDelegate(Browser* browser) {
  WebStateDelegateImpl::CreateForBrowser(browser);
  WebStateDelegateImpl::FromBrowser(browser)->Attach();
}

void WebStateDelegateServiceImpl::DetachWebStateDelegate(Browser* browser) {
  WebStateDelegateImpl::FromBrowser(browser)->Detach();
}
