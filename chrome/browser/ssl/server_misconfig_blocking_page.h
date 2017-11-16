// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_SERVER_MISCONFIG_BLOCKING_PAGE_H_
#define CHROME_BROWSER_SSL_SERVER_MISCONFIG_BLOCKING_PAGE_H_

#include <memory>

#include "chrome/browser/ssl/ssl_cert_reporter.h"
#include "components/security_interstitials/content/security_interstitial_page.h"
#include "net/ssl/ssl_info.h"

using content::InterstitialPageDelegate;

namespace security_interstitials {
class ServerMisconfigUi;
}  // namespace security_interstitials

class ServerMisconfigBlockingPage
    : public security_interstitials::SecurityInterstitialPage {
 public:
  // Interstitial type, used in tests.
  static InterstitialPageDelegate::TypeID kTypeForTesting;

  // If the blocking page isn't shown, the caller is responsible for cleaning
  // up the blocking page. Otherwise, the interstitial takes ownership when
  // shown.
  ServerMisconfigBlockingPage(content::WebContents* web_contents,
                              const GURL& request_url,
                              const net::SSLInfo& ssl_info);

  ~ServerMisconfigBlockingPage() override;

  // InterstitialPageDelegate method:
  InterstitialPageDelegate::TypeID GetTypeForTesting() const override;

  void SetSSLCertReporterForTesting(
      std::unique_ptr<SSLCertReporter> ssl_cert_reporter);

 protected:
  // InterstitialPageDelegate implementation:
  void CommandReceived(const std::string& command) override;
  // void OverrideEntry(content::NavigationEntry* entry) override;
  // void OverrideRendererPrefs(content::RendererPreferences* prefs) override;

  // SecurityInterstitialPage implementation:
  bool ShouldCreateNewNavigation() const override;
  void PopulateInterstitialStrings(
      base::DictionaryValue* load_time_data) override;

 private:
  const std::unique_ptr<security_interstitials::ServerMisconfigUi>
      server_misconfig_ui_;

  const net::SSLInfo ssl_info_;

  DISALLOW_COPY_AND_ASSIGN(ServerMisconfigBlockingPage);
};

#endif  // CHROME_BROWSER_SSL_SERVER_MISCONFIG_BLOCKING_PAGE_H_
