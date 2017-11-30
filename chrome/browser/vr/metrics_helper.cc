// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/metrics_helper.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "net/base/network_change_notifier.h"

namespace vr {

namespace {

constexpr char kStatusVr[] = "VR.Component.Assets.Status.OnEnter.Total";
constexpr char kStatusVrBrowsing[] =
    "VR.Component.Assets.Status.OnEnter.Browsing";
constexpr char kStatusWebVr[] =
    "VR.Component.Assets.Status.OnEnter.WebVRPresentation";
constexpr char kLatencyVrBrowsing[] =
    "VR.Component.Assets.DurationUntilReady.OnEnter.Browsing";
constexpr char kLatencyWebVr[] =
    "VR.Component.Assets.DurationUntilReady.OnEnter.WebVRPresentation";
constexpr char kNetworkConnectionTypeRegisterComponent[] =
    "VR.NetworkConnectionType.OnRegisterComponent";
constexpr char kNetworkConnectionTypeVr[] =
    "VR.NetworkConnectionType.OnEnter.Total";
constexpr char kNetworkConnectionTypeVrBrowsing[] =
    "VR.NetworkConnectionType.OnEnter.Browsing";
constexpr char kNetworkConnectionTypeWebVr[] =
    "VR.NetworkConnectionType.OnEnter.WebVRPresentation";

const auto kMinLatency = base::TimeDelta::FromMilliseconds(500);
const auto kMaxLatency = base::TimeDelta::FromHours(1);
constexpr size_t kLatencyBucketCount = 100;

// Ensure that this stays in sync with ComponentStatus in enums.xml.
enum class ComponentStatus : int {
  kReady = 0,
  kUnreadyOther = 1,

  // This must be last.
  kCount,
};

void LogStatus(Mode mode, ComponentStatus status) {
  switch (mode) {
    case Mode::kVr:
      UMA_HISTOGRAM_ENUMERATION(kStatusVr, status, ComponentStatus::kCount);
      return;
    case Mode::kVrBrowsing:
      UMA_HISTOGRAM_ENUMERATION(kStatusVrBrowsing, status,
                                ComponentStatus::kCount);
      return;
    case Mode::kWebVr:
      UMA_HISTOGRAM_ENUMERATION(kStatusWebVr, status, ComponentStatus::kCount);
      return;
    default:
      NOTIMPLEMENTED();
      return;
  }
}

void LogLatency(Mode mode, const base::TimeDelta& latency) {
  switch (mode) {
    case Mode::kVrBrowsing:
      UMA_HISTOGRAM_CUSTOM_TIMES(kLatencyVrBrowsing, latency, kMinLatency,
                                 kMaxLatency, kLatencyBucketCount);
      return;
    case Mode::kWebVr:
      UMA_HISTOGRAM_CUSTOM_TIMES(kLatencyWebVr, latency, kMinLatency,
                                 kMaxLatency, kLatencyBucketCount);
      return;
    default:
      NOTIMPLEMENTED();
      return;
  }
}

void LogConnectionType(Mode mode,
                       net::NetworkChangeNotifier::ConnectionType type) {
  switch (mode) {
    case Mode::kVr:
      UMA_HISTOGRAM_ENUMERATION(
          kNetworkConnectionTypeVr, type,
          net::NetworkChangeNotifier::ConnectionType::CONNECTION_LAST + 1);
      return;
    case Mode::kVrBrowsing:
      UMA_HISTOGRAM_ENUMERATION(
          kNetworkConnectionTypeVrBrowsing, type,
          net::NetworkChangeNotifier::ConnectionType::CONNECTION_LAST + 1);
      return;
    case Mode::kWebVr:
      UMA_HISTOGRAM_ENUMERATION(
          kNetworkConnectionTypeWebVr, type,
          net::NetworkChangeNotifier::ConnectionType::CONNECTION_LAST + 1);
      return;
    default:
      NOTIMPLEMENTED();
      return;
  }
}

}  // namespace

MetricsHelper::MetricsHelper() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

MetricsHelper::~MetricsHelper() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void MetricsHelper::OnComponentReady() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  component_ready_ = true;
  auto now = base::Time::Now();
  LogLatencyIfWaited(Mode::kVrBrowsing, now);
  LogLatencyIfWaited(Mode::kWebVr, now);
}

void MetricsHelper::OnEnter(Mode mode) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LogConnectionType(mode, net::NetworkChangeNotifier::GetConnectionType());
  auto& enter_time = GetEnterTime(mode);
  if (enter_time) {
    // While we are stopping the time between entering and component readiness
    // we pretend the user is waiting on the block screen. Do not report further
    // UMA metrics.
    return;
  }
  LogStatus(mode, component_ready_ ? ComponentStatus::kReady
                                   : ComponentStatus::kUnreadyOther);
  if (!component_ready_) {
    enter_time = base::Time::Now();
  }
}

void MetricsHelper::OnRegisteredComponent() {
  UMA_HISTOGRAM_ENUMERATION(
      kNetworkConnectionTypeRegisterComponent,
      net::NetworkChangeNotifier::GetConnectionType(),
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_LAST + 1);
}

base::Optional<base::Time>& MetricsHelper::GetEnterTime(Mode mode) {
  switch (mode) {
    case Mode::kVr:
      return enter_vr_time_;
    case Mode::kVrBrowsing:
      return enter_vr_browsing_time_;
    case Mode::kWebVr:
      return enter_web_vr_time_;
    default:
      NOTIMPLEMENTED();
      return enter_vr_time_;
  }
}

void MetricsHelper::LogLatencyIfWaited(Mode mode, const base::Time& now) {
  auto& enter_time = GetEnterTime(mode);
  if (enter_time) {
    LogLatency(mode, now - *enter_time);
    enter_time = base::nullopt;
  }
}

}  // namespace vr
