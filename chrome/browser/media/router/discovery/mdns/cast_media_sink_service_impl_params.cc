// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service_impl_params.h"

#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/media/router/media_router_feature.h"

namespace {

// Default values if field trial parameter is not specified.
constexpr int kDefaultInitialDelayInMilliSeconds = 15 * 1000;  // 15 seconds
// TODO(zhaobin): Remove this when we switch to use max delay instead of max
// number of retry attempts to decide when to stop retry.

constexpr int kDefaultMaxRetryAttempts = 3;

constexpr double kDefaultExponential = 1.0;

constexpr int kDefaultConnectTimeoutInSeconds = 10;

constexpr int kDefaultPingIntervalInSeconds = 5;

constexpr int kDefaultLivenessTimeoutInSeconds =
    kDefaultPingIntervalInSeconds * 2;

constexpr int kDefaultDynamicTimeoutDeltaInSeconds = 0;

}  // namespace

namespace media_router {

// static
const char CastChannelRetryParams::kParamNameInitialDelayInMilliSeconds[] =
    "initial_delay_in_ms";
const char CastChannelRetryParams::kParamNameMaxRetryAttempts[] =
    "max_retry_attempts";
const char CastChannelRetryParams::kParamNameExponential[] = "exponential";

CastChannelRetryParams::CastChannelRetryParams()
    : initial_delay_in_milliseconds(kDefaultInitialDelayInMilliSeconds),
      max_retry_attempts(kDefaultMaxRetryAttempts),
      multiply_factor(kDefaultExponential) {}
CastChannelRetryParams::~CastChannelRetryParams() = default;

bool CastChannelRetryParams::operator==(
    const CastChannelRetryParams& other) const {
  return initial_delay_in_milliseconds == other.initial_delay_in_milliseconds &&
         max_retry_attempts == other.max_retry_attempts &&
         multiply_factor == other.multiply_factor;
}

// static
CastChannelRetryParams CastChannelRetryParams::GetFromFieldTrialParam() {
  CastChannelRetryParams params;
  params.max_retry_attempts = base::GetFieldTrialParamByFeatureAsInt(
      kEnableCastDiscovery, kParamNameMaxRetryAttempts,
      kDefaultMaxRetryAttempts);
  params.initial_delay_in_milliseconds = base::GetFieldTrialParamByFeatureAsInt(
      kEnableCastDiscovery, kParamNameInitialDelayInMilliSeconds,
      kDefaultInitialDelayInMilliSeconds);
  params.multiply_factor = base::GetFieldTrialParamByFeatureAsDouble(
      kEnableCastDiscovery, kParamNameExponential, kDefaultExponential);

  DVLOG(2) << "Parameters: "
           << " [initial_delay_ms]: " << params.initial_delay_in_milliseconds
           << " [max_retry_attempts]: " << params.max_retry_attempts
           << " [exponential]: " << params.multiply_factor;

  return params;
}

// static
const char CastChannelOpenParams::kParamNameConnectTimeoutInSeconds[] =
    "connect_timeout_in_seconds";
const char CastChannelOpenParams::kParamNamePingIntervalInSeconds[] =
    "ping_interval_in_seconds";
const char CastChannelOpenParams::kParamNameLivenessTimeoutInSeconds[] =
    "liveness_timeout_in_seconds";
const char CastChannelOpenParams::kParamNameDynamicTimeoutDeltaInSeconds[] =
    "dynamic_timeout_delta_in_seconds";

CastChannelOpenParams::CastChannelOpenParams()
    : connect_timeout_in_seconds(kDefaultConnectTimeoutInSeconds),
      ping_interval_in_seconds(kDefaultPingIntervalInSeconds),
      liveness_timeout_in_seconds(kDefaultLivenessTimeoutInSeconds),
      dynamic_timeout_delta_in_seconds(kDefaultDynamicTimeoutDeltaInSeconds) {}
CastChannelOpenParams::~CastChannelOpenParams() = default;

bool CastChannelOpenParams::operator==(
    const CastChannelOpenParams& other) const {
  return connect_timeout_in_seconds == other.connect_timeout_in_seconds &&
         dynamic_timeout_delta_in_seconds ==
             other.dynamic_timeout_delta_in_seconds &&
         liveness_timeout_in_seconds == other.liveness_timeout_in_seconds &&
         ping_interval_in_seconds == other.ping_interval_in_seconds;
}

// static
CastChannelOpenParams CastChannelOpenParams::GetFromFieldTrialParam() {
  CastChannelOpenParams params;
  params.connect_timeout_in_seconds = base::GetFieldTrialParamByFeatureAsInt(
      kEnableCastDiscovery, kParamNameConnectTimeoutInSeconds,
      kDefaultConnectTimeoutInSeconds);
  params.ping_interval_in_seconds = base::GetFieldTrialParamByFeatureAsInt(
      kEnableCastDiscovery, kParamNamePingIntervalInSeconds,
      kDefaultPingIntervalInSeconds);
  params.liveness_timeout_in_seconds = base::GetFieldTrialParamByFeatureAsInt(
      kEnableCastDiscovery, kParamNameLivenessTimeoutInSeconds,
      kDefaultLivenessTimeoutInSeconds);
  params.dynamic_timeout_delta_in_seconds =
      base::GetFieldTrialParamByFeatureAsInt(
          kEnableCastDiscovery, kParamNameDynamicTimeoutDeltaInSeconds,
          kDefaultDynamicTimeoutDeltaInSeconds);

  DVLOG(2) << "Parameters: "
           << " [connect_timeout]: " << params.connect_timeout_in_seconds
           << " [ping_interval]: " << params.ping_interval_in_seconds
           << " [liveness_timeout]: " << params.liveness_timeout_in_seconds
           << " [dynamic_timeout_delta]: "
           << params.dynamic_timeout_delta_in_seconds;

  return params;
}

}  // namespace media_router
