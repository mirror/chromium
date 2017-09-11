// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/web_view_sync_initializer.h"

#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/signin/core/browser/account_fetcher_service.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "ios/web_view/internal/signin/web_view_account_consistency_service_factory.h"
#include "ios/web_view/internal/signin/web_view_account_fetcher_service_factory.h"
#include "ios/web_view/internal/signin/web_view_account_reconcilor_factory.h"
#include "ios/web_view/internal/signin/web_view_account_tracker_service_factory.h"
#include "ios/web_view/internal/signin/web_view_gaia_cookie_manager_service_factory.h"
#include "ios/web_view/internal/signin/web_view_oauth2_token_service_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_client_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_error_controller_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_manager_factory.h"
#include "ios/web_view/internal/web_view_browser_state.h"

namespace ios_web_view {

const char kDummyExtensionScheme[] = ":no-extension-scheme:";
const char* const kNonWildcardDomainNonPortSchemes[] = {kDummyExtensionScheme};
const size_t kNonWildcardDomainNonPortSchemesSize =
    arraysize(kNonWildcardDomainNonPortSchemes);

// static
void WebViewSyncInitializer::InitializePreMain() {
  WebViewAccountConsistencyServiceFactory::GetInstance();
  WebViewAccountFetcherServiceFactory::GetInstance();
  WebViewAccountReconcilorFactory::GetInstance();
  WebViewAccountTrackerServiceFactory::GetInstance();
  WebViewGaiaCookieManagerServiceFactory::GetInstance();
  WebViewOAuth2TokenServiceFactory::GetInstance();
  WebViewSigninClientFactory::GetInstance();
  WebViewSigninErrorControllerFactory::GetInstance();
  WebViewSigninManagerFactory::GetInstance();

  ContentSettingsPattern::SetNonWildcardDomainNonPortSchemes(
      kNonWildcardDomainNonPortSchemes, kNonWildcardDomainNonPortSchemesSize);
}

// static
void WebViewSyncInitializer::InitializeServicesForBrowserState(
    WebViewBrowserState* browser_state) {}

// static
void WebViewSyncInitializer::RegisterLocalPrefs(PrefRegistrySimple* registry) {
  SigninManagerBase::RegisterPrefs(registry);
}

// static
void WebViewSyncInitializer::RegisterBrowserPrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  HostContentSettingsMap::RegisterProfilePrefs(registry);
}

}  // namespace ios_web_view
