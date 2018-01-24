// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/settings/cros_settings_test_api.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"

namespace chromeos {

// static
void CrosSettingsTestApi::InitializeForTest() {
  CrosSettings::Initialize(g_browser_process->local_state(),
                           static_cast<policy::BrowserPolicyConnectorChromeOS*>(
                               g_browser_process->browser_policy_connector()));
}

// static
std::unique_ptr<CrosSettings> CrosSettingsTestApi::CreateForTest(
    DeviceSettingsService* device_settings_service) {
  return std::make_unique<CrosSettings>(
      g_browser_process->local_state(),
      static_cast<policy::BrowserPolicyConnectorChromeOS*>(
          g_browser_process->browser_policy_connector()),
      device_settings_service ? device_settings_service
                              : DeviceSettingsService::Get());
}

}  // namespace chromeos
