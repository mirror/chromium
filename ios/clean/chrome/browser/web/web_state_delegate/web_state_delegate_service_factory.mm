// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/web/web_state_delegate/web_state_delegate_service_factory.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/ui/browser_list/browser_list.h"
#import "ios/clean/chrome/browser/web/web_state_delegate/web_state_delegate_service_impl.h"
#import "ios/web/public/certificate_policy_cache.h"
#import "ios/web/public/web_state/session_certificate_policy_cache.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// static
WebStateDelegateService* WebStateDelegateServiceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<WebStateDelegateService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
WebStateDelegateServiceFactory* WebStateDelegateServiceFactory::GetInstance() {
  return base::Singleton<WebStateDelegateServiceFactory>::get();
}

WebStateDelegateServiceFactory::WebStateDelegateServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "WebStateDelegateService",
          BrowserStateDependencyManager::GetInstance()) {}

WebStateDelegateServiceFactory::~WebStateDelegateServiceFactory() {}

std::unique_ptr<KeyedService>
WebStateDelegateServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  return base::MakeUnique<WebStateDelegateServiceImpl>(
      BrowserList::FromBrowserState(browser_state));
}

web::BrowserState* WebStateDelegateServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateOwnInstanceInIncognito(context);
}
