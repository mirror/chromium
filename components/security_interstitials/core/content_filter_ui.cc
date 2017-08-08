// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/security_interstitials/core/content_filter_ui.h"

#include "base/i18n/time_formatting.h"
#include "components/security_interstitials/core/common_string_util.h"
#include "components/security_interstitials/core/metrics_helper.h"
#include "components/ssl_errors/error_info.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace security_interstitials {

ContentFilterUI::ContentFilterUI(const GURL& request_url,
                                 int cert_error,
                                 const net::SSLInfo& ssl_info,
                                 ControllerClient* controller)
    : request_url_(request_url),
      cert_error_(cert_error),
      ssl_info_(ssl_info),
      controller_(controller) {
  controller_->metrics_helper()->RecordUserInteraction(
      security_interstitials::MetricsHelper::TOTAL_VISITS);
}

ContentFilterUI::~ContentFilterUI() {
  controller_->metrics_helper()->RecordShutdownMetrics();
}

// TODO(sperigo): Fill in placeholder strings.
void ContentFilterUI::PopulateStringsForHTML(
    base::DictionaryValue* load_time_data) {
  CHECK(load_time_data);

  std::string content_filter_name = ssl_info_.cert->issuer().common_name;

  // Shared with other SSL errors.
  common_string_util::PopulateSSLLayoutStrings(cert_error_, load_time_data);
  common_string_util::PopulateSSLDebuggingStrings(
      ssl_info_, base::Time::NowFromSystemTime(), load_time_data);

  load_time_data->SetString("tabTitle", std::string());
  load_time_data->SetString("heading", std::string());
  load_time_data->SetString("primaryParagraph", std::string());

  base::string16 url(common_string_util::GetFormattedHostName(request_url_));

  load_time_data->SetBoolean("overridable", false);
  load_time_data->SetBoolean("hide_primary_button", true);
  load_time_data->SetString("explanationParagraph", std::string());
  load_time_data->SetString("primaryButtonText", std::string());
  load_time_data->SetString("finalParagraph", std::string());
}

void ContentFilterUI::HandleCommand(SecurityInterstitialCommands command) {
  switch (command) {
    case CMD_DONT_PROCEED:
      controller_->GoBack();
      break;
    case CMD_DO_REPORT:
      controller_->SetReportingPreference(true);
      break;
    case CMD_DONT_REPORT:
      controller_->SetReportingPreference(false);
      break;
    case CMD_SHOW_MORE_SECTION:
      controller_->metrics_helper()->RecordUserInteraction(
          security_interstitials::MetricsHelper::SHOW_ADVANCED);
      break;
    case CMD_OPEN_DATE_SETTINGS:
      if (!controller_->CanLaunchDateAndTimeSettings())
        NOTREACHED() << "This platform does not support date settings";
      controller_->metrics_helper()->RecordUserInteraction(
          security_interstitials::MetricsHelper::OPEN_TIME_SETTINGS);
      controller_->LaunchDateAndTimeSettings();
      break;
    case CMD_OPEN_REPORTING_PRIVACY:
      controller_->OpenExtendedReportingPrivacyPolicy();
      break;
    case CMD_OPEN_WHITEPAPER:
      controller_->OpenExtendedReportingWhitepaper();
      break;
    case CMD_PROCEED:
    case CMD_OPEN_HELP_CENTER:
    case CMD_RELOAD:
    case CMD_OPEN_DIAGNOSTIC:
    case CMD_OPEN_LOGIN:
    case CMD_REPORT_PHISHING_ERROR:
      // Not supported by the bad clock error page.
      NOTREACHED() << "Unsupported command: " << command;
    case CMD_ERROR:
    case CMD_TEXT_FOUND:
    case CMD_TEXT_NOT_FOUND:
      // Commands are only for testing.
      NOTREACHED() << "Unexpected command: " << command;
  }
}

}  // namespace security_interstitials
