// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/signin/ios_web_view_signin_client.h"

#include "components/signin/core/browser/signin_cookie_changed_subscription.h"
#include "components/signin/core/browser/signin_header_helper.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

IOSWebViewSigninClient::IOSWebViewSigninClient(
    PrefService* pref_service,
    net::URLRequestContextGetter* url_request_context,
    SigninErrorController* signin_error_controller,
    scoped_refptr<content_settings::CookieSettings> cookie_settings,
    scoped_refptr<HostContentSettingsMap> host_content_settings_map,
    scoped_refptr<TokenWebData> token_web_data)
    : OAuth2TokenService::Consumer("ios_web_view_signin_client"),
      pref_service_(pref_service),
      url_request_context_(url_request_context),
      signin_error_controller_(signin_error_controller),
      cookie_settings_(cookie_settings),
      host_content_settings_map_(host_content_settings_map),
      token_web_data_(token_web_data) {
  signin_error_controller_->AddObserver(this);
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

IOSWebViewSigninClient::~IOSWebViewSigninClient() {
  signin_error_controller_->RemoveObserver(this);
}

void IOSWebViewSigninClient::Shutdown() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void IOSWebViewSigninClient::OnSignedOut() {}

std::string IOSWebViewSigninClient::GetProductVersion() {
  // TODO(crbug.com/768689): Implement this method with appropriate values.
  return "";
}

base::Time IOSWebViewSigninClient::GetInstallDate() {
  // TODO(crbug.com/768689): Implement this method with appropriate values.
  return base::Time::FromTimeT(0);
}

scoped_refptr<TokenWebData> IOSWebViewSigninClient::GetDatabase() {
  return token_web_data_;
}

PrefService* IOSWebViewSigninClient::GetPrefs() {
  return pref_service_;
}

net::URLRequestContextGetter* IOSWebViewSigninClient::GetURLRequestContext() {
  return url_request_context_;
}

void IOSWebViewSigninClient::DoFinalInit() {}

bool IOSWebViewSigninClient::CanRevokeCredentials() {
  return true;
}

std::string IOSWebViewSigninClient::GetSigninScopedDeviceId() {
  return GetOrCreateScopedDeviceIdPref(GetPrefs());
}

bool IOSWebViewSigninClient::ShouldMergeSigninCredentialsIntoCookieJar() {
  return false;
}

bool IOSWebViewSigninClient::IsFirstRun() const {
  return false;
}

bool IOSWebViewSigninClient::AreSigninCookiesAllowed() {
  return signin::SettingsAllowSigninCookies(cookie_settings_.get());
}

void IOSWebViewSigninClient::AddContentSettingsObserver(
    content_settings::Observer* observer) {
  host_content_settings_map_->AddObserver(observer);
}

void IOSWebViewSigninClient::RemoveContentSettingsObserver(
    content_settings::Observer* observer) {
  host_content_settings_map_->RemoveObserver(observer);
}

std::unique_ptr<SigninClient::CookieChangedSubscription>
IOSWebViewSigninClient::AddCookieChangedCallback(
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

void IOSWebViewSigninClient::DelayNetworkCall(const base::Closure& callback) {
  // Don't bother if we don't have any kind of network connection.
  if (net::NetworkChangeNotifier::IsOffline()) {
    delayed_callbacks_.push_back(callback);
  } else {
    callback.Run();
  }
}

std::unique_ptr<GaiaAuthFetcher> IOSWebViewSigninClient::CreateGaiaAuthFetcher(
    GaiaAuthConsumer* consumer,
    const std::string& source,
    net::URLRequestContextGetter* getter) {
  return base::MakeUnique<GaiaAuthFetcher>(consumer, source, getter);
}

void IOSWebViewSigninClient::OnGetTokenInfoResponse(
    std::unique_ptr<base::DictionaryValue> token_info) {
  oauth_request_.reset();
}

void IOSWebViewSigninClient::OnOAuthError() {
  // Ignore the failure.  It's not essential and we'll try again next time.
  oauth_request_.reset();
}

void IOSWebViewSigninClient::OnNetworkError(int response_code) {
  // Ignore the failure.  It's not essential and we'll try again next time.
  oauth_request_.reset();
}

void IOSWebViewSigninClient::OnGetTokenSuccess(
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

void IOSWebViewSigninClient::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  // Ignore the failure.  It's not essential and we'll try again next time.
  oauth_request_.reset();
}

void IOSWebViewSigninClient::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  if (type >= net::NetworkChangeNotifier::ConnectionType::CONNECTION_NONE)
    return;

  for (const base::Closure& callback : delayed_callbacks_)
    callback.Run();

  delayed_callbacks_.clear();
}

void IOSWebViewSigninClient::OnErrorChanged() {}

void IOSWebViewSigninClient::SetAuthenticationController(
    CWVAuthenticationController* authentication_controller) {
  DCHECK(!authentication_controller || !authentication_controller_.get());
  authentication_controller_.reset(authentication_controller);
}

CWVAuthenticationController*
IOSWebViewSigninClient::GetAuthenticationController() {
  return authentication_controller_.get();
}
