// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_IOS_CHROME_SIGNIN_CLIENT_H_
#define IOS_CHROME_BROWSER_SIGNIN_IOS_CHROME_SIGNIN_CLIENT_H_

#include "components/signin/ios/browser/ios_signin_client.h"

namespace ios {
class ChromeBrowserState;
}

class IOSChromeSigninClient : public IOSSigninClient {
 public:
  IOSChromeSigninClient(ios::ChromeBrowserState* browser_state,
                        SigninErrorController* signin_error_controller,
                        content_settings::CookieSettings* cookie_settings,
                        HostContentSettingsMap* host_content_settings_map,
                        scoped_refptr<TokenWebData> token_web_data);

  // SigninClient implementation.
  void OnSignedOut() override;
  base::Time GetInstallDate() override;
  std::unique_ptr<GaiaAuthFetcher> CreateGaiaAuthFetcher(
      GaiaAuthConsumer* consumer,
      const std::string& source,
      net::URLRequestContextGetter* getter) override;
  // Returns a string describing the chrome version environment. Version format:
  // <Build Info> <OS> <Version number> (<Last change>)<channel or "-devel">
  // If version information is unavailable, returns "invalid."
  std::string GetProductVersion() override;

  void OnSignedIn(const std::string& account_id,
                  const std::string& gaia_id,
                  const std::string& username,
                  const std::string& password) override;

  // SigninErrorController::Observer implementation.
  void OnErrorChanged() override;

 private:
  ios::ChromeBrowserState* browser_state_;
  SigninErrorController* signin_error_controller_;
  DISALLOW_COPY_AND_ASSIGN(IOSChromeSigninClient);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_IOS_CHROME_SIGNIN_CLIENT_H_
