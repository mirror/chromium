// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/security_interstitials/core/superfish_error_ui.h"

#include "components/security_interstitials/core/common_string_util.h"
#include "components/security_interstitials/core/controller_client.h"
#include "components/security_interstitials/core/metrics_helper.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace security_interstitials {

namespace {

// URL for Superfish-specific help page.
const char kHelpURL[] = "https://support.google.com/chrome/?p=superfish";

}  // namespace

SuperfishErrorUI::SuperfishErrorUI(
    const GURL& request_url,
    int cert_error,
    const net::SSLInfo& ssl_info,
    int display_options,  // Bitmask of SSLErrorOptionsMask values.
    const base::Time& time_triggered,
    ControllerClient* controller)
    : SSLErrorUI(request_url,
                 cert_error,
                 ssl_info,
                 display_options,
                 time_triggered,
                 GURL(),
                 controller) {}

void SuperfishErrorUI::PopulateStringsForHTML(
    base::DictionaryValue* load_time_data) {
  common_string_util::PopulateSSLDebuggingStrings(ssl_info(), time_triggered(),
                                                  load_time_data);

  load_time_data->SetKey("type", base::Value("SSL"));
  load_time_data->SetKey("errorCode",
                         base::Value(net::ErrorToString(cert_error())));
  load_time_data->SetKey("overridable", base::Value(false));
  load_time_data->SetKey("bad_clock", base::Value(false));
  load_time_data->SetKey("hide_primary_button", base::Value(true));
  load_time_data->SetKey(
      "tabTitle", base::Value(l10n_util::GetStringUTF16(IDS_SSL_V2_TITLE)));
  load_time_data->SetKey(
      "heading",
      base::Value(l10n_util::GetStringUTF16(IDS_SSL_SUPERFISH_HEADING)));
  load_time_data->SetKey("primaryParagraph",
                         base::Value(l10n_util::GetStringUTF16(
                             IDS_SSL_SUPERFISH_PRIMARY_PARAGRAPH)));

  // Fill in empty values for normal SSL error strings that aren't used on this
  // interstitial.
  load_time_data->SetKey("explanationParagraph", base::Value(std::string()));
  load_time_data->SetKey("primaryButtonText", base::Value(std::string()));
  load_time_data->SetKey("finalParagraph", base::Value(std::string()));
  load_time_data->SetKey("openDetails", base::Value(base::string16()));
  load_time_data->SetKey("closeDetails", base::Value(base::string16()));
}

void SuperfishErrorUI::HandleCommand(SecurityInterstitialCommand command) {
  // Override the Help Center link to point to a Superfish-specific page.
  if (command == CMD_OPEN_HELP_CENTER) {
    controller()->metrics_helper()->RecordUserInteraction(
        security_interstitials::MetricsHelper::SHOW_LEARN_MORE);
    controller()->OpenUrlInNewForegroundTab(GURL(kHelpURL));
    return;
  }
  SSLErrorUI::HandleCommand(command);
}

}  // namespace security_interstitials
