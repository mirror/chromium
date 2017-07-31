// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_OFF_HOURS_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_OFF_HOURS_CONTROLLER_H_

#include <memory>

#include "chrome/browser/chromeos/settings/device_settings_service.h"
#include "components/policy/core/common/cloud/cloud_policy_store.h"

namespace chromeos {

class DeviceOffHoursController {
 public:
  // Apply OffHours policy for device policies depends on intervals
  static std::unique_ptr<enterprise_management::ChromeDeviceSettingsProto>
  ApplyOffHoursMode(
      enterprise_management::ChromeDeviceSettingsProto* input_policies);
  static void ApplyOffHoursMode(policy::PolicyMap* policies);

  //  private:
};
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_OFF_HOURS_CONTROLLER_H_
