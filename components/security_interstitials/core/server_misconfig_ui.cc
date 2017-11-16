// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/security_interstitials/core/server_misconfig_ui.h"

#include "base/strings/utf_string_conversions.h"
#include "components/security_interstitials/core/common_string_util.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace security_interstitials {

ServerMisconfigUi::ServerMisconfigUi(const net::SSLInfo& ssl_info)
    : ssl_info_(ssl_info) {}

ServerMisconfigUi::~ServerMisconfigUi() {}

void ServerMisconfigUi::PopulateStringsForHTML(
    base::DictionaryValue* load_time_data) {
  CHECK(load_time_data);

  // Shared with other SSL errors.
  common_string_util::PopulateSSLDebuggingStrings(
      ssl_info_, base::Time::NowFromSystemTime(), load_time_data);

  // Set display booleans.
  load_time_data->SetString("type", "SSL");
  load_time_data->SetBoolean("overridable", false);
  load_time_data->SetBoolean("hide_primary_button", true);
  load_time_data->SetBoolean("bad_clock", false);

  // Set strings that are shared between enterprise and non-enterprise
  // interstitials.
  load_time_data->SetString("tabTitle",
                            l10n_util::GetStringUTF16(IDS_SSL_V2_TITLE));
  load_time_data->SetString("heading",
                            base::ASCIIToUTF16("Server Misconfiguration"));

  load_time_data->SetString("primaryParagraph",
                            "Something bad kind of happened");

  load_time_data->SetString("explanationParagraph", std::string());
  load_time_data->SetString("primaryButtonText", std::string());
  load_time_data->SetString("finalParagraph", std::string());
  load_time_data->SetString("openDetails", base::string16());
  load_time_data->SetString("closeDetails", base::string16());
}

}  // namespace security_interstitials
