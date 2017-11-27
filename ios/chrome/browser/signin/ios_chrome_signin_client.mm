// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/ios_chrome_signin_client.h"

#include "base/strings/utf_string_conversions.h"
#include "components/metrics/metrics_service.h"
#include "components/signin/core/browser/signin_cookie_changed_subscription.h"
#include "components/signin/core/browser/signin_header_helper.h"
#include "components/signin/ios/browser/account_consistency_service.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/browser_state_info_cache.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/signin/account_consistency_service_factory.h"
#include "ios/chrome/browser/signin/gaia_auth_fetcher_ios.h"
#include "ios/chrome/browser/web_data_service_factory.h"
#include "ios/chrome/common/channel_info.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

IOSChromeSigninClient::IOSChromeSigninClient(
    ios::ChromeBrowserState* browser_state,
    SigninErrorController* signin_error_controller,
    scoped_refptr<content_settings::CookieSettings> cookie_settings,
    scoped_refptr<HostContentSettingsMap> host_content_settings_map,
    scoped_refptr<TokenWebData> token_web_data)
    : OAuth2TokenService::Consumer("ios_chrome_signin_client"),
      browser_state_(browser_state),
      signin_error_controller_(signin_error_controller),
      cookie_settings_(cookie_settings),
      host_content_settings_map_(host_content_settings_map),
      token_web_data_(token_web_data) {
  signin_error_controller_->AddObserver(this);
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

IOSChromeSigninClient::~IOSChromeSigninClient() {
  signin_error_controller_->RemoveObserver(this);
}

void IOSChromeSigninClient::Shutdown() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

base::Time IOSChromeSigninClient::GetInstallDate() {
  return base::Time::FromTimeT(
      GetApplicationContext()->GetMetricsService()->GetInstallDate());
}

// Returns a string describing the chrome version environment. Version format:
// <Build Info> <OS> <Version number> (<Last change>)<channel or "-devel">
// If version information is unavailable, returns "invalid."
std::string IOSChromeSigninClient::GetProductVersion() {
  return GetVersionString();
}

void IOSChromeSigninClient::OnSignedIn(const std::string& account_id,
                                       const std::string& gaia_id,
                                       const std::string& username,
                                       const std::string& password) {
  ios::ChromeBrowserStateManager* browser_state_manager =
      GetApplicationContext()->GetChromeBrowserStateManager();
  BrowserStateInfoCache* cache =
      browser_state_manager->GetBrowserStateInfoCache();
  size_t index = cache->GetIndexOfBrowserStateWithPath(
      browser_state_->GetOriginalChromeBrowserState()->GetStatePath());
  if (index != std::string::npos) {
    cache->SetAuthInfoOfBrowserStateAtIndex(index, gaia_id,
                                            base::UTF8ToUTF16(username));
  }
}

void IOSChromeSigninClient::OnSignedOut() {
  BrowserStateInfoCache* cache = GetApplicationContext()
                                     ->GetChromeBrowserStateManager()
                                     ->GetBrowserStateInfoCache();
  size_t index = cache->GetIndexOfBrowserStateWithPath(
      browser_state_->GetOriginalChromeBrowserState()->GetStatePath());

  // If sign out occurs because Sync setup was in progress and the browser state
  // got deleted, then it is no longer in the cache.
  if (index == std::string::npos)
    return;

  cache->SetAuthInfoOfBrowserStateAtIndex(index, std::string(),
                                          base::string16());
}

scoped_refptr<TokenWebData> IOSChromeSigninClient::GetDatabase() {
  return token_web_data_;
}

PrefService* IOSChromeSigninClient::GetPrefs() {
  return browser_state_->GetPrefs();
}

net::URLRequestContextGetter* IOSChromeSigninClient::GetURLRequestContext() {
  return browser_state_->GetRequestContext();
}

void IOSChromeSigninClient::DoFinalInit() {}

bool IOSChromeSigninClient::CanRevokeCredentials() {
  return true;
}

std::string IOSChromeSigninClient::GetSigninScopedDeviceId() {
  return GetOrCreateScopedDeviceIdPref(GetPrefs());
}

bool IOSChromeSigninClient::ShouldMergeSigninCredentialsIntoCookieJar() {
  return false;
}

bool IOSChromeSigninClient::IsFirstRun() const {
  return false;
}

bool IOSChromeSigninClient::AreSigninCookiesAllowed() {
  return signin::SettingsAllowSigninCookies(cookie_settings_.get());
}

void IOSChromeSigninClient::AddContentSettingsObserver(
    content_settings::Observer* observer) {
  host_content_settings_map_->AddObserver(observer);
}

void IOSChromeSigninClient::RemoveContentSettingsObserver(
    content_settings::Observer* observer) {
  host_content_settings_map_->RemoveObserver(observer);
}

std::unique_ptr<SigninClient::CookieChangedSubscription>
IOSChromeSigninClient::AddCookieChangedCallback(
    const GURL& url,
    const std::string& name,
    const net::CookieStore::CookieChangedCallback& callback) {
  scoped_refptr<net::URLRequestContextGetter> context_getter =
      GetURLRequestContext();
  DCHECK(context_getter.get());
  std::unique_ptr<SigninCookieChangedSubscription> subscription(
      new SigninCookieChangedSubscription(context_getter, url, name, callback));
  return subscription;
}

void IOSChromeSigninClient::DelayNetworkCall(const base::Closure& callback) {
  // Don't bother if we don't have any kind of network connection.
  if (net::NetworkChangeNotifier::IsOffline()) {
    delayed_callbacks_.push_back(callback);
  } else {
    callback.Run();
  }
}

std::unique_ptr<GaiaAuthFetcher> IOSChromeSigninClient::CreateGaiaAuthFetcher(
    GaiaAuthConsumer* consumer,
    const std::string& source,
    net::URLRequestContextGetter* getter) {
  return std::make_unique<GaiaAuthFetcherIOS>(consumer, source, getter,
                                              browser_state_);
}

void IOSChromeSigninClient::PreGaiaLogout(base::OnceClosure callback) {
  AccountConsistencyService* accountConsistencyService =
      ios::AccountConsistencyServiceFactory::GetForBrowserState(browser_state_);
  accountConsistencyService->RemoveChromeConnectedCookies(std::move(callback));
}

void IOSChromeSigninClient::OnGetTokenInfoResponse(
    std::unique_ptr<base::DictionaryValue> token_info) {
  oauth_request_.reset();
}

void IOSChromeSigninClient::OnOAuthError() {
  // Ignore the failure.  It's not essential and we'll try again next time.
  oauth_request_.reset();
}

void IOSChromeSigninClient::OnNetworkError(int response_code) {
  // Ignore the failure.  It's not essential and we'll try again next time.
  oauth_request_.reset();
}

void IOSChromeSigninClient::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  // Exchange the access token for a handle that can be used for later
  // verification that the token is still valid (i.e. the password has not
  // been changed).
  if (!oauth_client_) {
    oauth_client_.reset(new gaia::GaiaOAuthClient(GetURLRequestContext()));
  }
  oauth_client_->GetTokenInfo(access_token, 3 /* retries */, this);
}

void IOSChromeSigninClient::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  // Ignore the failure.  It's not essential and we'll try again next time.
  oauth_request_.reset();
}

void IOSChromeSigninClient::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  if (type >= net::NetworkChangeNotifier::ConnectionType::CONNECTION_NONE)
    return;

  for (const base::Closure& callback : delayed_callbacks_)
    callback.Run();

  delayed_callbacks_.clear();
}

void IOSChromeSigninClient::OnErrorChanged() {
  BrowserStateInfoCache* cache = GetApplicationContext()
                                     ->GetChromeBrowserStateManager()
                                     ->GetBrowserStateInfoCache();
  size_t index = cache->GetIndexOfBrowserStateWithPath(
      browser_state_->GetOriginalChromeBrowserState()->GetStatePath());
  if (index == std::string::npos)
    return;

  cache->SetBrowserStateIsAuthErrorAtIndex(
      index, signin_error_controller_->HasError());
}
