// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/download_params.h"

#include "components/download/public/clients.h"

namespace download {

namespace {
const int kDefaultOptimalBatteryPercentage = 50;
}  // namespace

SchedulingParams::SchedulingParams()
    : cancel_time(base::Time::Max()),
      priority(Priority::DEFAULT),
      network_requirements(NetworkRequirements::NONE),
      battery_requirements(BatteryRequirements::BATTERY_INSENSITIVE),
      optimal_battery_percentage(kDefaultOptimalBatteryPercentage) {}

bool SchedulingParams::operator==(const SchedulingParams& rhs) const {
  return network_requirements == rhs.network_requirements &&
         battery_requirements == rhs.battery_requirements &&
         priority == rhs.priority && cancel_time == rhs.cancel_time;
}

RequestParams::RequestParams() : method("GET") {}

DownloadParams::DownloadParams() : client(DownloadClient::INVALID) {}

DownloadParams::DownloadParams(const DownloadParams& other) = default;

DownloadParams::~DownloadParams() = default;

}  // namespace download
