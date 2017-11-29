// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_IOS_CHROME_SIGNIN_CLIENT_H_
#define IOS_CHROME_BROWSER_SIGNIN_IOS_CHROME_SIGNIN_CLIENT_H_

#include <memory>

#include "base/macros.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/signin/core/browser/signin_client.h"
#include "components/signin/core/browser/signin_error_controller.h"
#include "components/signin/ios/browser/delay_callback_helper.h"
#include "net/url_request/url_request_context_getter.h"

namespace ios {
class ChromeBrowserState;
}

// Concrete implementation of SigninClient for //ios/chrome.
class IOSChromeSigninClient : public SigninClient,
                              public SigninErrorController::Observer {
 public:
  IOSChromeSigninClient(
      std::unique_ptr<DelayCallbackHelper> delay_callback_helper,
      ios::ChromeBrowserState* browser_state,
      SigninErrorController* signin_error_controller,
      scoped_refptr<content_settings::CookieSettings> cookie_settings,
      scoped_refptr<HostContentSettingsMap> host_content_settings_map,
      scoped_refptr<TokenWebData> token_web_data);
  ~IOSChromeSigninClient() override;

  // KeyedService implementation.
  void Shutdown() override;

  // SigninClient implementation.
  base::Time GetInstallDate() override;
  std::string GetProductVersion() override;
  void OnSignedIn(const std::string& account_id,
                  const std::string& gaia_id,
                  const std::string& username,
                  const std::string& password) override;
  std::unique_ptr<GaiaAuthFetcher> CreateGaiaAuthFetcher(
      GaiaAuthConsumer* consumer,
      const std::string& source,
      net::URLRequestContextGetter* getter) override;
  void PreGaiaLogout(base::OnceClosure callback) override;
  scoped_refptr<TokenWebData> GetDatabase() override;
  PrefService* GetPrefs() override;
  net::URLRequestContextGetter* GetURLRequestContext() override;
  void DoFinalInit() override;
  bool CanRevokeCredentials() override;
  std::string GetSigninScopedDeviceId() override;
  bool ShouldMergeSigninCredentialsIntoCookieJar() override;
  bool IsFirstRun() const override;
  bool AreSigninCookiesAllowed() override;
  void AddContentSettingsObserver(
      content_settings::Observer* observer) override;
  void RemoveContentSettingsObserver(
      content_settings::Observer* observer) override;
  std::unique_ptr<CookieChangedSubscription> AddCookieChangedCallback(
      const GURL& url,
      const std::string& name,
      const net::CookieStore::CookieChangedCallback& callback) override;
  void DelayNetworkCall(const base::Closure& callback) override;

  // SigninErrorController::Observer implementation.
  void OnErrorChanged() override;

 private:
  // SigninClient private implementation.
  void OnSignedOut() override;

  std::unique_ptr<DelayCallbackHelper> delay_callback_helper_;
  ios::ChromeBrowserState* browser_state_;
  SigninErrorController* signin_error_controller_;
  scoped_refptr<content_settings::CookieSettings> cookie_settings_;
  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;
  scoped_refptr<TokenWebData> token_web_data_;

  DISALLOW_COPY_AND_ASSIGN(IOSChromeSigninClient);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_IOS_CHROME_SIGNIN_CLIENT_H_
