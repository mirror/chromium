// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_IMPL_PARAMS_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_IMPL_PARAMS_H_

namespace media_router {

// Struct holds Finch field trial parameters controlling Cast channel retry
// strategy.
struct CastChannelRetryParams {
  // Initial delay (in ms) once backoff starts.
  int initial_delay_in_milliseconds;

  // Max retry attempts allowed when opening a Cast socket.
  int max_retry_attempts;

  // Factor by which the delay will be multiplied on each subsequent failure.
  double multiply_factor;

  // Parameter names;
  static const char kParamNameInitialDelayInMilliSeconds[];
  static const char kParamNameMaxRetryAttempts[];
  static const char kParamNameExponential[];

  CastChannelRetryParams();
  ~CastChannelRetryParams();
  bool operator==(const CastChannelRetryParams& other) const;

  static CastChannelRetryParams GetFromFieldTrialParam();
};

// Struct holds Finch field trial parameters controlling Cast channel open.
struct CastChannelOpenParams {
  // Connect timeout value when opening a Cast socket.
  int connect_timeout_in_seconds;

  // Amount of idle time to wait before pinging the Cast device.
  int ping_interval_in_seconds;

  // Amount of idle time to wait before disconnecting.
  int liveness_timeout_in_seconds;

  // Dynamic time out delta for connect timeout and liveness timeout. If
  // previous channel open operation with opening parameters (liveness timeout,
  // connect timeout) fails, next channel open will have parameters (liveness
  // timeout + delta, connect timeout + delta).
  int dynamic_timeout_delta_in_seconds;

  // Parameter names.
  static const char kParamNameConnectTimeoutInSeconds[];
  static const char kParamNamePingIntervalInSeconds[];
  static const char kParamNameLivenessTimeoutInSeconds[];
  static const char kParamNameDynamicTimeoutDeltaInSeconds[];

  CastChannelOpenParams();
  ~CastChannelOpenParams();
  bool operator==(const CastChannelOpenParams& other) const;

  static CastChannelOpenParams GetFromFieldTrialParam();
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_MDNS_CAST_MEDIA_SINK_SERVICE_IMPL_PARAMS_H_
