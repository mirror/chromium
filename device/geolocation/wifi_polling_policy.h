// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GEOLOCATION_WIFI_POLLING_POLICY_H_
#define DEVICE_GEOLOCATION_WIFI_POLLING_POLICY_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "device/geolocation/geolocation_export.h"

namespace device {

// Allows sharing and mocking of the update polling policy function.
class DEVICE_GEOLOCATION_EXPORT WifiPollingPolicy {
 public:
  virtual ~WifiPollingPolicy() = default;

  static void Initialize(std::unique_ptr<WifiPollingPolicy>);
  static void Shutdown();
  static WifiPollingPolicy* Get();
  static bool IsInitialized();

  // Calculates the new polling interval for wiFi scans, given the previous
  // interval and whether the last scan produced new results.
  virtual void UpdatePollingInterval(bool scan_results_differ) = 0;
  virtual int PollingInterval() = 0;
  virtual int NoWifiInterval() = 0;

 protected:
  WifiPollingPolicy() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(WifiPollingPolicy);
};

// Generic polling policy, constants are compile-time parameterized to allow
// tuning on a per-platform basis.
template <int DEFAULT_INTERVAL,
          int NO_CHANGE_INTERVAL,
          int TWO_NO_CHANGE_INTERVAL,
          int NO_WIFI_INTERVAL>
class GenericWifiPollingPolicy : public WifiPollingPolicy {
 public:
  GenericWifiPollingPolicy() : polling_interval_(DEFAULT_INTERVAL) {}

  // WifiPollingPolicy
  void UpdatePollingInterval(bool scan_results_differ) override {
    if (scan_results_differ) {
      polling_interval_ = DEFAULT_INTERVAL;
    } else if (polling_interval_ == DEFAULT_INTERVAL) {
      polling_interval_ = NO_CHANGE_INTERVAL;
    } else {
      DCHECK(polling_interval_ == NO_CHANGE_INTERVAL ||
             polling_interval_ == TWO_NO_CHANGE_INTERVAL);
      polling_interval_ = TWO_NO_CHANGE_INTERVAL;
    }
  }

  int PollingInterval() override { return ComputeInterval(polling_interval_); }

  int NoWifiInterval() override { return ComputeInterval(NO_WIFI_INTERVAL); }

 private:
  int ComputeInterval(int polling_interval) {
    base::Time now = base::Time::Now();

    // Scan immediately if this is our first scan.
    if (interval_complete_time_.is_null()) {
      interval_complete_time_ = now;
      return 0;
    }

    // Scan immediately if the last scan was more than one interval ago.
    base::TimeDelta interval_delta =
        base::TimeDelta::FromMilliseconds(polling_interval);
    base::Time complete_time = interval_complete_time_ + interval_delta;
    if (complete_time < now) {
      interval_complete_time_ = now;
      return 0;
    }

    // Otherwise, schedule the next scan after the polling interval.
    interval_complete_time_ = now + interval_delta;
    return polling_interval;
  }

  int polling_interval_;

  // Record the expected time that the current interval will be complete.
  base::Time interval_complete_time_;
};

}  // namespace device

#endif  // DEVICE_GEOLOCATION_WIFI_POLLING_POLICY_H_
