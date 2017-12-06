// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/chrome/certificate_error_override_manager.h"

#include "base/values.h"
#include "chrome/test/chromedriver/chrome/devtools_client.h"
#include "chrome/test/chromedriver/chrome/status.h"

CertificateErrorOverrideManager::CertificateErrorOverrideManager(
    DevToolsClient* client,
    OverrideMode initial_mode)
    : client_(client), mode_(initial_mode) {
  client_->AddListener(this);
}

CertificateErrorOverrideManager::~CertificateErrorOverrideManager() {}

Status CertificateErrorOverrideManager::OverrideCertificateErrors(
    OverrideMode mode) {
  Status status = ApplyOverride(mode);
  if (status.IsOk())
    mode_ = mode;
  return status;
}

Status CertificateErrorOverrideManager::OnConnected(DevToolsClient* client) {
  return ApplyOverride(mode_);
}

Status CertificateErrorOverrideManager::OnEvent(
    DevToolsClient* client,
    const std::string& method,
    const base::DictionaryValue& params) {
  if (mode_ == OverrideMode::kClearOverride)
    return Status(kOk);

  if (method == "Security.certificateError") {
    int eventId;
    if (!params.GetInteger("eventId", &eventId)) {
      return Status(
          kInvalidArgument,
          "Security.certificateError event is missing required field eventId.");
    }

    // Ignore the certificate error by continuing the navigation.
    DCHECK_EQ(OverrideMode::kIgnoreErrors, mode_);
    base::DictionaryValue params;
    params.SetInteger("eventId", eventId);
    params.SetString("action", "continue");
    fprintf(stderr, "allowing navigation\n");
    return client_->SendCommand("Security.handleCertificateError", params);
  }
  return Status(kOk);
}

Status CertificateErrorOverrideManager::ApplyOverride(OverrideMode mode) {
  base::DictionaryValue params, empty_params;

  bool override = mode != OverrideMode::kClearOverride;
  if (override) {
    Status status = client_->SendCommand("Security.enable", empty_params);
    if (status.IsError())
      return status;

    params.SetBoolean("override", override);
    fprintf(stderr, "applying override\n");
    return client_->SendCommand("Security.setOverrideCertificateErrors",
                                params);
  } else {
    fprintf(stderr, "clearing override\n");
    return client_->SendCommand("Security.disable", empty_params);
  }
}
