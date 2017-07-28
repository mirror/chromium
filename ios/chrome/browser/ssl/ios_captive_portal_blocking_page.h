// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SSL_IOS_CAPTIVE_PORTAL_BLOCKING_PAGE_H_
#define IOS_CHROME_BROWSER_SSL_IOS_CAPTIVE_PORTAL_BLOCKING_PAGE_H_

#include "base/callback.h"
#include "ios/chrome/browser/interstitials/ios_security_interstitial_page.h"
#include "url/gurl.h"

class IOSCaptivePortalBlockingPage : public IOSSecurityInterstitialPage {
 public:
  ~IOSCaptivePortalBlockingPage() override;

  // Creates a captive portal blocking page. If the blocking page isn't shown,
  // the caller is responsible for cleaning up the blocking page, otherwise the
  // interstitial takes ownership when shown.
  IOSCaptivePortalBlockingPage(web::WebState* web_state,
                               const GURL& request_url,
                               const GURL& landing_url,
                               const base::Callback<void(bool)>& callback);

 private:
  // IOSSecurityInterstitialPage overrides:
  bool ShouldCreateNewNavigation() const override;
  void PopulateInterstitialStrings(base::DictionaryValue*) const override;
  void AfterShow() override;
  void CommandReceived(const std::string& command) override;

  // The landing page url for the captive portal network.
  const GURL landing_url_;

  // |callback| is run with a parameter of true to re-try the navigation which
  // triggered this blocking page.
  base::Callback<void(bool)> callback_;

  DISALLOW_COPY_AND_ASSIGN(IOSCaptivePortalBlockingPage);
};

#endif  // IOS_CHROME_BROWSER_SSL_IOS_CAPTIVE_PORTAL_BLOCKING_PAGE_H_
