// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_SCHEDULER_DEVICE_STATUS_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_SCHEDULER_DEVICE_STATUS_H_

#include "components/download/public/download_params.h"

namespace download {

// Network status used in download service.
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
  DeviceStatus();
  struct Result {
    Result();
    bool MeetRequirements() const;
    bool meet_battery_requirement;
    bool meet_network_requirement;
  };

  BatteryStatus battery_status;
  NetworkStatus network_status;

  bool operator==(const DeviceStatus& rhs);

  // Check the scheduling parameter to see if the current device status
  // meets all the condition.
  Result MeetCondition(const SchedulingParams& params) const;
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_SCHEDULER_DEVICE_STATUS_H_
