// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/download_params.h"

#include "components/download/public/clients.h"

namespace download {

SchedulingParams::SchedulingParams()
    : cancel_time(base::Time::Max()),
      priority(Priority::DEFAULT),
      network_requirements(NetworkRequirements::NONE),
      battery_requirements(BatteryRequirements::BATTERY_INSENSITIVE) {}

bool SchedulingParams::operator==(const SchedulingParams& rhs) const {
  return network_requirements == rhs.network_requirements &&
         battery_requirements == rhs.battery_requirements &&
         priority == rhs.priority && cancel_time == rhs.cancel_time;
}

RequestParams::RequestParams() : method("GET") {}

DownloadParams::DownloadParams() : client(DownloadClient::INVALID) {}

DownloadParams::DownloadParams(const DownloadParams& other)
    : client(other.client),
      guid(other.guid),
      callback(other.callback),
      scheduling_params(other.scheduling_params),
      request_params(other.request_params) {
  if (other.has_traffic_annotation())
    set_traffic_annotation(other.get_traffic_annotation());
}

DownloadParams::~DownloadParams() = default;

}  // namespace download
