// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/scheduler/device_status.h"

namespace download {

DeviceStatus::Result::Result()
    : meet_battery_requirement(false), meet_network_requirement(false) {}

bool DeviceStatus::Result::MeetRequirements() const {
  return meet_battery_requirement && meet_network_requirement;
}

DeviceStatus::DeviceStatus()
    : battery_status(BatteryStatus::NOT_CHARGING),
      network_status(NetworkStatus::DISCONNECTED) {}

bool DeviceStatus::operator==(const DeviceStatus& rhs) {
  return network_status == rhs.network_status &&
         battery_status == rhs.battery_status;
}

DeviceStatus::Result DeviceStatus::MeetCondition(
    const SchedulingParams& params) const {
  DeviceStatus::Result result;

  switch (params.battery_requirements) {
    case SchedulingParams::BatteryRequirements::BATTERY_INSENSITIVE:
      result.meet_battery_requirement = true;
      break;
    case SchedulingParams::BatteryRequirements::BATTERY_SENSITIVE:
      result.meet_battery_requirement =
          battery_status == BatteryStatus::CHARGING;
      break;
    default:
      NOTREACHED();
  }
  switch (params.network_requirements) {
    case SchedulingParams::NetworkRequirements::NONE:
      result.meet_network_requirement =
          network_status != NetworkStatus::DISCONNECTED;
      break;
    case SchedulingParams::NetworkRequirements::OPTIMISTIC:
    case SchedulingParams::NetworkRequirements::UNMETERED:
      result.meet_network_requirement =
          network_status == NetworkStatus::UNMETERED;
      break;
    default:
      NOTREACHED();
  }
  return result;
}

}  // namespace download
