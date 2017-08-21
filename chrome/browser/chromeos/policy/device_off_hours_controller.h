// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_

#include <memory>
#include <set>

#include "chrome/browser/chromeos/policy/proto/chrome_device_policy.pb.h"

namespace policy {

class DeviceOffHoursController {
 private:
  // Clear |off_policies| from |input_policies|
  static enterprise_management::ChromeDeviceSettingsProto ClearPolicies(
      enterprise_management::ChromeDeviceSettingsProto* input_policies,
      std::set<std::string> off_policies);
};
}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_
