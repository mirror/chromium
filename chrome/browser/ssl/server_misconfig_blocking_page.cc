// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/server_misconfig_blocking_page.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/interstitials/chrome_controller_client.h"
#include "chrome/browser/interstitials/chrome_metrics_helper.h"
#include "components/security_interstitials/core/server_misconfig_ui.h"
#include "content/public/browser/interstitial_page_delegate.h"

namespace {

const char kMetricsName[] = "server_misconfig";

// TODO: Let's move this into like a seperate class.
std::unique_ptr<ChromeMetricsHelper> CreateMetricsHelper(
    content::WebContents* web_contents,
    const GURL& request_url) {
  // Set up the metrics helper for the ServerMisconfigUi.
  security_interstitials::MetricsHelper::ReportDetails reporting_info;
  reporting_info.metric_prefix = kMetricsName;
  std::unique_ptr<ChromeMetricsHelper> metrics_helper =
      base::MakeUnique<ChromeMetricsHelper>(web_contents, request_url,
                                            reporting_info, kMetricsName);
  metrics_helper.get()->StartRecordingCaptivePortalMetrics(false);
  return metrics_helper;
}

}  // namespace

// static
InterstitialPageDelegate::TypeID ServerMisconfigBlockingPage::kTypeForTesting =
    &ServerMisconfigBlockingPage::kTypeForTesting;

ServerMisconfigBlockingPage::ServerMisconfigBlockingPage(
    content::WebContents* web_contents,
    const GURL& request_url,
    const net::SSLInfo& ssl_info)
    : SecurityInterstitialPage(
          web_contents,
          request_url,
          base::MakeUnique<ChromeControllerClient>(
              web_contents,
              CreateMetricsHelper(web_contents, request_url))),
      server_misconfig_ui_(
          new security_interstitials::ServerMisconfigUi(ssl_info)),
      ssl_info_(ssl_info) {}

ServerMisconfigBlockingPage::~ServerMisconfigBlockingPage() {}

InterstitialPageDelegate::TypeID
ServerMisconfigBlockingPage::GetTypeForTesting() const {
  return ServerMisconfigBlockingPage::kTypeForTesting;
}

void ServerMisconfigBlockingPage::CommandReceived(const std::string& command) {}

bool ServerMisconfigBlockingPage::ShouldCreateNewNavigation() const {
  // TODO: check if that's the case
  return true;
}

void ServerMisconfigBlockingPage::PopulateInterstitialStrings(
    base::DictionaryValue* load_time_data) {
  server_misconfig_ui_->PopulateStringsForHTML(load_time_data);
}
