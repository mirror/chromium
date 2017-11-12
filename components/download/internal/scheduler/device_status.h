// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_SCHEDULER_DEVICE_STATUS_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_SCHEDULER_DEVICE_STATUS_H_

#include "components/download/public/download_params.h"

namespace download {

// Battery status used in download service.
enum class BatteryStatus {
  CHARGING = 0,
  NOT_CHARGING = 1,
};

// NetworkStatus should mostly one to one map to
// SchedulingParams::NetworkRequirements. Has coarser granularity than
// network connection type.
enum class NetworkStatus {
  DISCONNECTED = 0,
  UNMETERED = 1,  // WIFI or Ethernet.
  METERED = 2,    // Mobile networks.
};

// Contains battery and network status.
struct DeviceStatus {
  // The optimal battery percentage to schedule the background task that runs
  // download service.
  static const int kOptimalSchedulingBatteryPercentage;

  // The optimal battery percentage to start battery sensitive downloads.
  // Slightly lower than |kOptimalSchedulingBatteryPercentage| in case the
  // background task runs at the lowest possible battery level.
  static const int kOptimalDownloadBatteryPercentage;

  // Optimal battery percentage that always start the download service.
  static const int kBatteryPercentageAlwaysDownload;

  DeviceStatus();
  DeviceStatus(BatteryStatus battery, NetworkStatus network);
  DeviceStatus(BatteryStatus battery,
               int battery_percentage,
               NetworkStatus network);

  struct Result {
    Result();
    bool MeetsRequirements() const;
    bool meets_battery_requirement;
    bool meets_network_requirement;
  };

  BatteryStatus battery_status;

  // Battery percentage which is in the range of [0, 100].
  int battery_percentage;

  NetworkStatus network_status;

  bool operator==(const DeviceStatus& rhs) const;
  bool operator!=(const DeviceStatus& rhs) const;

  // Returns if the current device status meets all the conditions defined in
  // the scheduling parameters.
  Result MeetsCondition(const SchedulingParams& params) const;
};

// The criteria when the background download task should start.
struct Criteria {
  Criteria();
  Criteria(bool requires_battery_charging, bool requires_unmetered_network);
  Criteria(bool requires_battery_charging,
           bool requires_unmetered_network,
           int optimal_battery_percentage);
  bool operator==(const Criteria& other) const;
  bool requires_battery_charging;
  bool requires_unmetered_network;
  int optimal_battery_percentage;
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_SCHEDULER_DEVICE_STATUS_H_
