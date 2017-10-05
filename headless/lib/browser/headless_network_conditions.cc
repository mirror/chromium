// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/headless_network_conditions.h"

namespace headless {

HeadlessNetworkConditions::HeadlessNetworkConditions()
    : HeadlessNetworkConditions(false) {}

HeadlessNetworkConditions::HeadlessNetworkConditions(bool offline)
    : HeadlessNetworkConditions(offline, 0, 0, 0) {}

HeadlessNetworkConditions::HeadlessNetworkConditions(bool offline,
                                                     double latency,
                                                     double download_throughput,
                                                     double upload_throughput)
    : offline_(offline),
      latency_(latency),
      download_throughput_(download_throughput),
      upload_throughput_(upload_throughput) {}

HeadlessNetworkConditions::~HeadlessNetworkConditions() {}

bool HeadlessNetworkConditions::IsThrottling() const {
  return !offline_ && ((latency_ != 0) || (download_throughput_ != 0.0) ||
                       (upload_throughput_ != 0));
}

}  // namespace headless
