// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_

#include <memory>

#include "base/values.h"
#include "chrome/browser/chromeos/policy/proto/chrome_device_policy.pb.h"

namespace policy {

class DeviceOffHoursController {
 public:
  // Convert DeviceOffHoursProto to DictionaryValue
  static std::unique_ptr<base::DictionaryValue> ConvertToValue(
      const enterprise_management::DeviceOffHoursProto& container);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_
