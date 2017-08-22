// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/signin/web_view_account_consistency_service_factory.h"

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/signin/ios/browser/account_consistency_service.h"
#include "ios/web_view/internal/content_settings/web_view_cookie_settings_factory.h"
#include "ios/web_view/internal/signin/web_view_account_reconcilor_factory.h"
#include "ios/web_view/internal/signin/web_view_gaia_cookie_manager_service_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_client_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_manager_factory.h"
#include "ios/web_view/internal/web_view_browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

WebViewAccountConsistencyServiceFactory::
    WebViewAccountConsistencyServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "AccountConsistencyService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(WebViewAccountReconcilorFactory::GetInstance());
  DependsOn(WebViewCookieSettingsFactory::GetInstance());
  DependsOn(WebViewGaiaCookieManagerServiceFactory::GetInstance());
  DependsOn(WebViewSigninClientFactory::GetInstance());
  DependsOn(WebViewSigninManagerFactory::GetInstance());
}

WebViewAccountConsistencyServiceFactory::
    ~WebViewAccountConsistencyServiceFactory() {}

// static
AccountConsistencyService*
WebViewAccountConsistencyServiceFactory::GetForBrowserState(
    WebViewBrowserState* browser_state) {
  return static_cast<AccountConsistencyService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
WebViewAccountConsistencyServiceFactory*
WebViewAccountConsistencyServiceFactory::GetInstance() {
  return base::Singleton<WebViewAccountConsistencyServiceFactory>::get();
}

void WebViewAccountConsistencyServiceFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  AccountConsistencyService::RegisterPrefs(registry);
}

std::unique_ptr<KeyedService>
WebViewAccountConsistencyServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  WebViewBrowserState* browserState =
      WebViewBrowserState::FromBrowserState(context);
  return base::MakeUnique<AccountConsistencyService>(
      browserState,
      WebViewAccountReconcilorFactory::GetForBrowserState(browserState),
      WebViewCookieSettingsFactory::GetForBrowserState(browserState),
      WebViewGaiaCookieManagerServiceFactory::GetForBrowserState(browserState),
      WebViewSigninClientFactory::GetForBrowserState(browserState),
      WebViewSigninManagerFactory::GetForBrowserState(browserState));
}

}  // namespace ios_web_view
