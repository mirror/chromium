// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_

#include <memory>

#include "base/values.h"
#include "chrome/browser/chromeos/policy/proto/chrome_device_policy.pb.h"

namespace policy {
namespace off_hours {

// Return DictionaryValue format:
// { "timezone" : string,
//   "intervals" : list of Intervals,
//   "ignored_policies" : string list }
// Interval dictionary format:
// { "start" : WeeklyTime,
//   "end" : WeeklyTime }
// WeeklyTime dictionary format:
// { "weekday" : int,
//   "time" : int }
std::unique_ptr<base::DictionaryValue> ConvertToValue(
    const enterprise_management::DeviceOffHoursProto& container);

}  // namespace off_hours
}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_
