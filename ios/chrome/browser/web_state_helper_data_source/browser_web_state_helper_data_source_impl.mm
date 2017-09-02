// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web_state_helper_data_source/browser_web_state_helper_data_source_impl.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/ui/browser_list/browser_list.h"
#import "ios/chrome/browser/web_state_helper_data_source/browser_web_state_delegate_helper.h"
#import "ios/chrome/browser/web_state_helper_data_source/web_state_user_data_creation_helper.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {}  // namespace

BrowserWebStateHelperDataSourceImpl::BrowserWebStateHelperDataSourceImpl()
    : browser_list_(nullptr) {}

BrowserWebStateHelperDataSourceImpl::~BrowserWebStateHelperDataSourceImpl() {
  // ShutDown() should be called before destruction.
  DCHECK(!browser_list_);
}

#pragma mark - BrowserListObserver

void BrowserWebStateHelperDataSourceImpl::OnBrowserCreated(
    BrowserList* browser_list,
    Browser* browser) {
  DCHECK_EQ(browser_list, browser_list_);
  AttachHelpers(browser);
}

#pragma mark - BrowserWebStateHelperDataSource

void BrowserWebStateHelperDataSourceImpl::ProvideHelpersToBrowsers(
    BrowserList* browser_list) {
  DCHECK(!browser_list_ && browser_list);
  browser_list_ = browser_list;
  for (int index = 0; index < browser_list_->count(); ++index) {
    AttachHelpers(browser_list_->GetBrowserAtIndex(index));
  }
  browser_list_->AddObserver(this);
}

#pragma mark - KeyedService

void BrowserWebStateHelperDataSourceImpl::Shutdown() {
  browser_list_->RemoveObserver(this);
  browser_list_ = nullptr;
}

#pragma mark - Private

void BrowserWebStateHelperDataSourceImpl::AttachHelpers(Browser* browser) {
  // Attach helpers.
  DCHECK(browser);
  browser->AddWebStateHelper(
      base::MakeUnique<WebStateUserDataCreationHelper>());
  browser->AddWebStateHelper(
      base::MakeUnique<BrowserWebStateDelegateHelper>(browser));
}
