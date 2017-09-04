// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/signin/web_view_account_reconcilor_factory.h"

#include <utility>

#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/signin/core/browser/account_reconcilor.h"
#include "ios/web_view/internal/signin/web_view_gaia_cookie_manager_service_factory.h"
#include "ios/web_view/internal/signin/web_view_oauth2_token_service_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_client_factory.h"
#include "ios/web_view/internal/signin/web_view_signin_manager_factory.h"
#include "ios/web_view/internal/web_view_browser_state.h"

namespace ios_web_view {

WebViewAccountReconcilorFactory::WebViewAccountReconcilorFactory()
    : BrowserStateKeyedServiceFactory(
          "AccountReconcilor",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(WebViewGaiaCookieManagerServiceFactory::GetInstance());
  DependsOn(WebViewOAuth2TokenServiceFactory::GetInstance());
  DependsOn(WebViewSigninClientFactory::GetInstance());
  DependsOn(WebViewSigninManagerFactory::GetInstance());
}

WebViewAccountReconcilorFactory::~WebViewAccountReconcilorFactory() {}

// static
AccountReconcilor* WebViewAccountReconcilorFactory::GetForBrowserState(
    ios_web_view::WebViewBrowserState* browser_state) {
  return static_cast<AccountReconcilor*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
WebViewAccountReconcilorFactory*
WebViewAccountReconcilorFactory::GetInstance() {
  return base::Singleton<WebViewAccountReconcilorFactory>::get();
}

std::unique_ptr<KeyedService>
WebViewAccountReconcilorFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  WebViewBrowserState* browser_state =
      WebViewBrowserState::FromBrowserState(context);
  std::unique_ptr<AccountReconcilor> reconcilor(new AccountReconcilor(
      WebViewOAuth2TokenServiceFactory::GetForBrowserState(browser_state),
      WebViewSigninManagerFactory::GetForBrowserState(browser_state),
      WebViewSigninClientFactory::GetForBrowserState(browser_state),
      WebViewGaiaCookieManagerServiceFactory::GetForBrowserState(
          browser_state)));
  reconcilor->Initialize(true /* start_reconcile_if_tokens_available */);
  return std::move(reconcilor);
}

}  // namespace ios_web_view
