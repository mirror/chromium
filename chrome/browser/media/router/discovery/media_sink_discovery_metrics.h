// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MEDIA_SINK_DISCOVERY_METRICS_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MEDIA_SINK_DISCOVERY_METRICS_H_

#include <memory>

#include "base/time/clock.h"
#include "base/time/time.h"

namespace media_router {

// Possible values for Cast channel connect results. Keep in sync with
// BooleanSuccess in analysis/uma/configs/chrome/histograms.xml.
enum class MediaRouterChannelConnectResults {
  FAILURE = 0,
  SUCCESS = 1,

  // Note = Add entries only immediately above this line.
  TOTAL_COUNT = 2
};

// Possible values for sink discovery type.
enum class MediaRouterSinkDiscoveryType {
  DIAL = 0,
  MDNS = 1,
  MDNSDIAL = 2,
  DIALMDNS = 3,

  // Note = Add entries only immediately above this line.
  TOTAL_COUNT = 4
};

// Possible values for channel open errors.
enum class MediaRouterChannelError {
  UNKNOWN = 0,
  AUTHENTICATION = 1,
  CONNECT = 2,
  GENERAL_CERTIFICATE = 3,
  CERTIFICATE_TIMING = 4,
  NETWORK = 5,
  CONNECT_TIMEOUT = 6,
  PING_TIMEOUT = 7,

  // Note = Add entries only immediately above this line.
  TOTAL_COUNT = 8
};

class DeviceCountMetrics {
 public:
  DeviceCountMetrics();
  ~DeviceCountMetrics();

  // Records device counts if last record was more than one hour ago.
  void RecordDeviceCountsIfNeeded(size_t available_device_count,
                                  size_t known_device_count);

  // Allows tests to swap in a fake clock.
  void SetClockForTest(std::unique_ptr<base::Clock> clock);

 protected:
  // Record device counts.
  virtual void RecordDeviceCounts(size_t available_device_count,
                                  size_t known_device_count) = 0;

 private:
  base::Time device_count_metrics_record_time_;

  std::unique_ptr<base::Clock> clock_;
};

// Metrics for DIAL device counts.
class DialDeviceCountMetrics : public DeviceCountMetrics {
 public:
  static const char kHistogramDialAvailableDeviceCount[];
  static const char kHistogramDialKnownDeviceCount[];

  void RecordDeviceCounts(size_t available_device_count,
                          size_t known_device_count) override;
};

// Metrics for Cast device counts.
class CastDeviceCountMetrics : public DeviceCountMetrics {
 public:
  static const char kHistogramCastKnownDeviceCount[];
  static const char kHistogramCastConnectedDeviceCount[];

  void RecordDeviceCounts(size_t available_device_count,
                          size_t known_device_count) override;
};

class CastAnalytics {
 public:
  static constexpr char const kHistogramCastChannelConnectResult[] =
      "MediaRouter.Cast.Channel.ConnectResult";
  static constexpr char const kHistogramCastDiscoveryType[] =
      "MediaRouter.Cast.Discovery.Type";
  static constexpr char const kHistogramCastChannelError[] =
      "MediaRouter.Cast.Channel.Error";
  static constexpr char const kHistogramCastMdnsChannelOpenSuccess[] =
      "MediaRouter.Cast.Mdns.Channel.Open_Success";
  static constexpr char const kHistogramCastMdnsChannelOpenFailure[] =
      "MediaRouter.Cast.Mdns.Channel.Open_Failure";

  static void RecordCastChannelConnectResult(
      MediaRouterChannelConnectResults result);
  static void RecordDeviceDiscovery(MediaRouterSinkDiscoveryType type);
  static void RecordDeviceChannelError(MediaRouterChannelError channel_error);
  static void RecordDeviceChannelOpenDuration(bool success,
                                              const base::TimeDelta& duration);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MEDIA_SINK_DISCOVERY_METRICS_H_
