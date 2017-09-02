// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web_state_helper_data_source/browser_web_state_helper_data_source_factory.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/ui/browser_list/browser_list.h"
#import "ios/chrome/browser/web_state_helper_data_source/browser_web_state_helper_data_source_impl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// static
BrowserWebStateHelperDataSource*
BrowserWebStateHelperDataSourceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<BrowserWebStateHelperDataSource*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
BrowserWebStateHelperDataSourceFactory*
BrowserWebStateHelperDataSourceFactory::GetInstance() {
  return base::Singleton<BrowserWebStateHelperDataSourceFactory>::get();
}

BrowserWebStateHelperDataSourceFactory::BrowserWebStateHelperDataSourceFactory()
    : BrowserStateKeyedServiceFactory(
          "BrowserWebStateHelperDataSource",
          BrowserStateDependencyManager::GetInstance()) {}

BrowserWebStateHelperDataSourceFactory::
    ~BrowserWebStateHelperDataSourceFactory() {}

std::unique_ptr<KeyedService>
BrowserWebStateHelperDataSourceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return base::MakeUnique<BrowserWebStateHelperDataSourceImpl>();
}

web::BrowserState* BrowserWebStateHelperDataSourceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateOwnInstanceInIncognito(context);
}
