// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/signin/web_view_signin_client_impl.h"

#include <stddef.h>

#include <utility>

#include "base/command_line.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_cookie_changed_subscription.h"
#include "components/signin/core/browser/signin_header_helper.h"
#include "components/signin/ios/browser/profile_oauth2_token_service_ios_provider.h"
#include "google_apis/gaia/gaia_auth_fetcher.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/gaia_urls.h"
#include "ios/web_view/internal/app/application_context.h"
#include "ios/web_view/internal/content_settings/web_view_cookie_settings_factory.h"
#include "ios/web_view/internal/content_settings/web_view_host_content_settings_map_factory.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

WebViewSigninClientImpl::WebViewSigninClientImpl(
    ios_web_view::WebViewBrowserState* browser_state,
    SigninErrorController* signin_error_controller)
    : OAuth2TokenService::Consumer("web_view_signin_client_impl"),
      browser_state_(browser_state),
      signin_error_controller_(signin_error_controller) {
  signin_error_controller_->AddObserver(this);
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

WebViewSigninClientImpl::~WebViewSigninClientImpl() {
  signin_error_controller_->RemoveObserver(this);
}

void WebViewSigninClientImpl::Shutdown() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void WebViewSigninClientImpl::DoFinalInit() {}

// static
bool WebViewSigninClientImpl::AllowsSigninCookies(
    ios_web_view::WebViewBrowserState* browser_state) {
  scoped_refptr<content_settings::CookieSettings> cookie_settings =
      ios_web_view::WebViewCookieSettingsFactory::GetForBrowserState(
          browser_state);
  return signin::SettingsAllowSigninCookies(cookie_settings.get());
}

PrefService* WebViewSigninClientImpl::GetPrefs() {
  return browser_state_->GetPrefs();
}

scoped_refptr<TokenWebData> WebViewSigninClientImpl::GetDatabase() {
  return nullptr;
}

bool WebViewSigninClientImpl::CanRevokeCredentials() {
  return true;
}

std::string WebViewSigninClientImpl::GetSigninScopedDeviceId() {
  return GetOrCreateScopedDeviceIdPref(GetPrefs());
}

void WebViewSigninClientImpl::OnSignedOut() {}

net::URLRequestContextGetter* WebViewSigninClientImpl::GetURLRequestContext() {
  return browser_state_->GetRequestContext();
}

bool WebViewSigninClientImpl::ShouldMergeSigninCredentialsIntoCookieJar() {
  return false;
}

std::string WebViewSigninClientImpl::GetProductVersion() {
  return "";
}

bool WebViewSigninClientImpl::IsFirstRun() const {
  return false;
}

base::Time WebViewSigninClientImpl::GetInstallDate() {
  return base::Time::FromTimeT(0);
}

bool WebViewSigninClientImpl::AreSigninCookiesAllowed() {
  return AllowsSigninCookies(browser_state_);
}

void WebViewSigninClientImpl::AddContentSettingsObserver(
    content_settings::Observer* observer) {
  ios_web_view::WebViewHostContentSettingsMapFactory::GetForBrowserState(
      browser_state_)
      ->AddObserver(observer);
}

void WebViewSigninClientImpl::RemoveContentSettingsObserver(
    content_settings::Observer* observer) {
  ios_web_view::WebViewHostContentSettingsMapFactory::GetForBrowserState(
      browser_state_)
      ->RemoveObserver(observer);
}

std::unique_ptr<SigninClient::CookieChangedSubscription>
WebViewSigninClientImpl::AddCookieChangedCallback(
    const GURL& url,
    const std::string& name,
    const net::CookieStore::CookieChangedCallback& callback) {
  scoped_refptr<net::URLRequestContextGetter> context_getter =
      browser_state_->GetRequestContext();
  DCHECK(context_getter.get());
  std::unique_ptr<SigninCookieChangedSubscription> subscription(
      new SigninCookieChangedSubscription(context_getter, url, name, callback));
  // TODO(crbug.com/703565): remove std::move() once Xcode 9.0+ is required.
  return std::move(subscription);
}

void WebViewSigninClientImpl::OnSignedIn(const std::string& account_id,
                                         const std::string& gaia_id,
                                         const std::string& username,
                                         const std::string& password) {}

void WebViewSigninClientImpl::OnErrorChanged() {}

void WebViewSigninClientImpl::OnGetTokenInfoResponse(
    std::unique_ptr<base::DictionaryValue> token_info) {
  oauth_request_.reset();
}

void WebViewSigninClientImpl::OnOAuthError() {
  // Ignore the failure.  It's not essential and we'll try again next time.
  oauth_request_.reset();
}

void WebViewSigninClientImpl::OnNetworkError(int response_code) {
  // Ignore the failure.  It's not essential and we'll try again next time.
  oauth_request_.reset();
}

void WebViewSigninClientImpl::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  // Exchange the access token for a handle that can be used for later
  // verification that the token is still valid (i.e. the password has not
  // been changed).
  if (!oauth_client_) {
    oauth_client_.reset(
        new gaia::GaiaOAuthClient(browser_state_->GetRequestContext()));
  }
  oauth_client_->GetTokenInfo(access_token, 3 /* retries */, this);
}

void WebViewSigninClientImpl::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  // Ignore the failure.  It's not essential and we'll try again next time.
  oauth_request_.reset();
}

void WebViewSigninClientImpl::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  if (type >= net::NetworkChangeNotifier::ConnectionType::CONNECTION_NONE)
    return;

  for (const base::Closure& callback : delayed_callbacks_)
    callback.Run();

  delayed_callbacks_.clear();
}

void WebViewSigninClientImpl::DelayNetworkCall(const base::Closure& callback) {
  // Don't bother if we don't have any kind of network connection.
  if (net::NetworkChangeNotifier::IsOffline()) {
    delayed_callbacks_.push_back(callback);
  } else {
    callback.Run();
  }
}

std::unique_ptr<GaiaAuthFetcher> WebViewSigninClientImpl::CreateGaiaAuthFetcher(
    GaiaAuthConsumer* consumer,
    const std::string& source,
    net::URLRequestContextGetter* getter) {
  return base::MakeUnique<GaiaAuthFetcher>(consumer, source, getter);
}
