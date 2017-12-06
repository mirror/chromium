// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_CHROME_CERTIFICATE_ERROR_OVERRIDE_MANAGER_H_
#define CHROME_TEST_CHROMEDRIVER_CHROME_CERTIFICATE_ERROR_OVERRIDE_MANAGER_H_

#include <string>

#include "base/macros.h"
#include "chrome/test/chromedriver/chrome/devtools_event_listener.h"

// Facilitates handling certificate errors via DevTools. Currently only supports
// overriding certificate errors by ignoring all certificate errors.
class CertificateErrorOverrideManager : public DevToolsEventListener {
 public:
  enum class OverrideMode { kIgnoreErrors, kClearOverride };

  CertificateErrorOverrideManager(DevToolsClient* client,
                                  OverrideMode initial_mode);
  ~CertificateErrorOverrideManager() override;

  Status OverrideCertificateErrors(OverrideMode mode);

  // Overridden from DevToolsEventListener:
  Status OnConnected(DevToolsClient* client) override;
  Status OnEvent(DevToolsClient* client,
                 const std::string& method,
                 const base::DictionaryValue& params) override;

 private:
  Status ApplyOverride(OverrideMode mode);

  DevToolsClient* client_;
  OverrideMode mode_;

  DISALLOW_COPY_AND_ASSIGN(CertificateErrorOverrideManager);
};

#endif  // CHROME_TEST_CHROMEDRIVER_CHROME_CERTIFICATE_ERROR_OVERRIDE_MANAGER_H_
