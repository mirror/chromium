// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/scheduler/device_status.h"

namespace download {

const int DeviceStatus::kDefaultOptimalBatteryPercentage = 50;
const int DeviceStatus::kMinimumBatteryPercentage = 10;

DeviceStatus::Result::Result()
    : meets_battery_requirement(false), meets_network_requirement(false) {}

bool DeviceStatus::Result::MeetsRequirements() const {
  return meets_battery_requirement && meets_network_requirement;
}

DeviceStatus::DeviceStatus()
    : battery_status(BatteryStatus::NOT_CHARGING),
      battery_percentage(0),
      network_status(NetworkStatus::DISCONNECTED) {}

DeviceStatus::DeviceStatus(BatteryStatus battery, NetworkStatus network)
    : DeviceStatus(battery, 0, network) {}

DeviceStatus::DeviceStatus(BatteryStatus battery,
                           int battery_percentage,
                           NetworkStatus network)
    : battery_status(battery),
      battery_percentage(battery_percentage),
      network_status(network) {}

bool DeviceStatus::operator==(const DeviceStatus& rhs) const {
  return network_status == rhs.network_status &&
         battery_status == rhs.battery_status &&
         battery_percentage == rhs.battery_percentage;
}

bool DeviceStatus::operator!=(const DeviceStatus& rhs) const {
  return !(*this == rhs);
}

DeviceStatus::Result DeviceStatus::MeetsCondition(
    const SchedulingParams& params) const {
  DeviceStatus::Result result;

  switch (params.battery_requirements) {
    case SchedulingParams::BatteryRequirements::BATTERY_INSENSITIVE:
      result.meets_battery_requirement = true;
      break;
    case SchedulingParams::BatteryRequirements::BATTERY_SENSITIVE:
      result.meets_battery_requirement =
          battery_status == BatteryStatus::CHARGING ||
          battery_percentage >= kDefaultOptimalBatteryPercentage;
      break;
    case SchedulingParams::BatteryRequirements::BATTERY_CHARGING:
      result.meets_battery_requirement =
          battery_status == BatteryStatus::CHARGING;
      break;
    default:
      NOTREACHED();
  }
  switch (params.network_requirements) {
    case SchedulingParams::NetworkRequirements::NONE:
      result.meets_network_requirement =
          network_status != NetworkStatus::DISCONNECTED;
      break;
    case SchedulingParams::NetworkRequirements::OPTIMISTIC:
    case SchedulingParams::NetworkRequirements::UNMETERED:
      result.meets_network_requirement =
          network_status == NetworkStatus::UNMETERED;
      break;
    default:
      NOTREACHED();
  }
  return result;
}

Criteria::Criteria()
    : Criteria(true, true, DeviceStatus::kDefaultOptimalBatteryPercentage) {}

Criteria::Criteria(bool requires_battery_charging,
                   bool requires_unmetered_network)
    : Criteria(requires_battery_charging,
               requires_unmetered_network,
               DeviceStatus::kDefaultOptimalBatteryPercentage) {}

Criteria::Criteria(bool requires_battery_charging,
                   bool requires_unmetered_network,
                   int optimal_battery_percentage)
    : requires_battery_charging(requires_battery_charging),
      requires_unmetered_network(requires_unmetered_network),
      optimal_battery_percentage(optimal_battery_percentage) {}

bool Criteria::operator==(const Criteria& other) const {
  return requires_battery_charging == other.requires_battery_charging &&
         requires_unmetered_network == other.requires_unmetered_network &&
         optimal_battery_percentage == other.optimal_battery_percentage;
}

}  // namespace download
