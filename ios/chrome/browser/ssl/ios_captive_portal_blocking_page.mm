// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ssl/ios_captive_portal_blocking_page.h"

#include "base/i18n/rtl.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "components/captive_portal/captive_portal_detector.h"
#include "components/security_interstitials/core/controller_client.h"
#include "components/strings/grit/components_strings.h"
#include "components/url_formatter/url_formatter.h"
#include "ios/chrome/browser/ssl/captive_portal_detector_tab_helper.h"
#include "ios/chrome/browser/ssl/captive_portal_detector_tab_helper_delegate.h"
#import "net/base/mac/url_conversions.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Events for UMA.
enum CaptivePortalBlockingPageEvent {
  SHOW_ALL,
  OPEN_LOGIN_PAGE,
  CAPTIVE_PORTAL_BLOCKING_PAGE_EVENT_COUNT
};

const char kInterstitialCaptivePortalHistogram[] =
    "interstitial.captive_portal";

IOSCaptivePortalBlockingPage::IOSCaptivePortalBlockingPage(
    web::WebState* web_state,
    const GURL& request_url,
    const GURL& landing_url,
    const base::Callback<void(bool)>& callback)
    : IOSSecurityInterstitialPage(web_state, request_url),
      landing_url_(landing_url),
      callback_(callback) {
  UMA_HISTOGRAM_ENUMERATION(kInterstitialCaptivePortalHistogram, SHOW_ALL,
                            CAPTIVE_PORTAL_BLOCKING_PAGE_EVENT_COUNT);
}

IOSCaptivePortalBlockingPage::~IOSCaptivePortalBlockingPage() {}

bool IOSCaptivePortalBlockingPage::ShouldCreateNewNavigation() const {
  return true;
}

void IOSCaptivePortalBlockingPage::PopulateInterstitialStrings(
    base::DictionaryValue* load_time_data) const {
  load_time_data->SetString("iconClass", "icon-offline");
  load_time_data->SetString("type", "CAPTIVE_PORTAL");
  load_time_data->SetBoolean("overridable", false);
  load_time_data->SetString(
      "primaryButtonText",
      l10n_util::GetStringUTF16(IDS_CAPTIVE_PORTAL_BUTTON_OPEN_LOGIN_PAGE));

  base::string16 tab_title =
      l10n_util::GetStringUTF16(IDS_CAPTIVE_PORTAL_HEADING_WIFI);
  load_time_data->SetString("tabTitle", tab_title);
  load_time_data->SetString("heading", tab_title);

  base::string16 paragraph;
  if (landing_url_.spec() ==
      captive_portal::CaptivePortalDetector::kDefaultURL) {
    // Captive portal may intercept requests without HTTP redirects, in which
    // case the login url would be the same as the captive portal detection url.
    // Don't show the login url in that case.
    paragraph = l10n_util::GetStringUTF16(
        IDS_CAPTIVE_PORTAL_PRIMARY_PARAGRAPH_NO_LOGIN_URL_WIFI);
  } else {
    // Portal redirection was done with HTTP redirects, so show the login URL.
    // If |languages| is empty, punycode in |login_host| will always be decoded.
    base::string16 login_host =
        url_formatter::IDNToUnicode(landing_url_.host());
    if (base::i18n::IsRTL())
      base::i18n::WrapStringWithLTRFormatting(&login_host);

    paragraph = l10n_util::GetStringFUTF16(
        IDS_CAPTIVE_PORTAL_PRIMARY_PARAGRAPH_WIFI, login_host);
  }
  load_time_data->SetString("primaryParagraph", paragraph);
  // Explicitly specify other expected fields to empty.
  load_time_data->SetString("openDetails", base::string16());
  load_time_data->SetString("closeDetails", base::string16());
  load_time_data->SetString("explanationParagraph", base::string16());
  load_time_data->SetString("finalParagraph", base::string16());
}

void IOSCaptivePortalBlockingPage::AfterShow() {}

void IOSCaptivePortalBlockingPage::CommandReceived(const std::string& command) {
  int command_num = 0;
  bool command_is_num = base::StringToInt(command, &command_num);
  DCHECK(command_is_num) << command;
  // Any command other than "open the login page" is ignored.
  if (command_num == security_interstitials::CMD_OPEN_LOGIN) {
    UMA_HISTOGRAM_ENUMERATION(kInterstitialCaptivePortalHistogram,
                              OPEN_LOGIN_PAGE,
                              CAPTIVE_PORTAL_BLOCKING_PAGE_EVENT_COUNT);

    CaptivePortalDetectorTabHelper* tab_helper =
        CaptivePortalDetectorTabHelper::FromWebState(web_state());
    [tab_helper->delegate()
                captivePortalTabHelper:tab_helper
        didSelectConnectWithLandingURL:net::NSURLWithGURL(landing_url_)];
  }
}
