// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SETTINGS_CROS_SETTINGS_TEST_API_H_
#define CHROME_BROWSER_CHROMEOS_SETTINGS_CROS_SETTINGS_TEST_API_H_

#include <memory>

#include "base/macros.h"

namespace chromeos {

class CrosSettings;
class DeviceSettingsService;

class CrosSettingsTestApi {
 public:
  // Initialize CrosSettings from g_browser_process. This assumes the test has
  // already configure g_browser_process (which is typically done in tests by
  // way of ScopedTestingLocalState.
  static void InitializeForTest();

  static std::unique_ptr<CrosSettings> CreateForTest(
      DeviceSettingsService* device_settings_service = nullptr);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(CrosSettingsTestApi);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SETTINGS_CROS_SETTINGS_TEST_API_H_
