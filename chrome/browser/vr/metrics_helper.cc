// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/metrics_helper.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/version.h"

namespace vr {

namespace {

static constexpr char kStatusVr[] = "VR.AssetsComponent.Status.OnEnter.VR";
static constexpr char kStatusVrBrowsing[] =
    "VR.AssetsComponent.Status.OnEnter.VRBrowsing";
static constexpr char kStatusWebVr[] =
    "VR.AssetsComponent.Status.OnEnter.WebVR";
static constexpr char kLatencyVrBrowsing[] =
    "VR.AssetsComponent.ReadyLatency.OnEnter.VRBrowsing";
static constexpr char kLatencyWebVr[] =
    "VR.AssetsComponent.ReadyLatency.OnEnter.WebVR";
constexpr char kComponentUpdateStatus[] = "VR.AssetsComponent.UpdateStatus";
constexpr char kAssetsLoadStatus[] = "VR.AssetsComponent.LoadStatus";
static const auto kMinLatency = base::TimeDelta::FromMilliseconds(500);
static const auto kMaxLatency = base::TimeDelta::FromHours(1);
static constexpr size_t kLatencyBucketCount = 100;

// Ensure that this stays in sync with VRAssetsComponentEnterStatus in
// enums.xml.
enum class AssetsComponentEnterStatus : int {
  kReady = 0,
  kUnreadyOther = 1,

  // This must be last.
  kCount,
};

static void LogStatus(Mode mode, AssetsComponentEnterStatus status) {
  switch (mode) {
    case Mode::kVr:
      UMA_HISTOGRAM_ENUMERATION(kStatusVr, status,
                                AssetsComponentEnterStatus::kCount);
      return;
    case Mode::kVrBrowsing:
      UMA_HISTOGRAM_ENUMERATION(kStatusVrBrowsing, status,
                                AssetsComponentEnterStatus::kCount);
      return;
    case Mode::kWebVr:
      UMA_HISTOGRAM_ENUMERATION(kStatusWebVr, status,
                                AssetsComponentEnterStatus::kCount);
      return;
    default:
      NOTIMPLEMENTED();
      return;
  }
}

static void LogLatency(Mode mode, const base::TimeDelta& latency) {
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

uint32_t EncodeVersionStatus(const base::Optional<base::Version>& version,
                             int status) {
  if (!version) {
    // Component version 0.0 is invalid. Thus, use it for when version is not
    // available.
    return status;
  }
  return version->components()[0] * 1000 * 1000 +
         version->components()[1] * 1000 + status;
}

}  // namespace

MetricsHelper::MetricsHelper() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

MetricsHelper::~MetricsHelper() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void MetricsHelper::OnComponentReady(const base::Version& version) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  component_ready_ = true;
  auto now = base::Time::Now();
  LogLatencyIfWaited(Mode::kVrBrowsing, now);
  LogLatencyIfWaited(Mode::kWebVr, now);
  OnComponentUpdated(AssetsComponentUpdateStatus::kSuccess, version);
}

void MetricsHelper::OnEnter(Mode mode) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto& enter_time = GetEnterTime(mode);
  if (enter_time) {
    // While we are stopping the time between entering and component readiness
    // we pretend the user is waiting on the block screen. Do not report further
    // UMA metrics.
    return;
  }
  LogStatus(mode, component_ready_ ? AssetsComponentEnterStatus::kReady
                                   : AssetsComponentEnterStatus::kUnreadyOther);
  if (!component_ready_) {
    enter_time = base::Time::Now();
  }
}

void MetricsHelper::OnComponentUpdated(
    AssetsComponentUpdateStatus status,
    const base::Optional<base::Version>& version) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  UMA_HISTOGRAM_SPARSE_SLOWLY(
      kComponentUpdateStatus,
      EncodeVersionStatus(version, static_cast<int>(status)));
}

void MetricsHelper::OnAssetsLoaded(AssetsLoadStatus status,
                                   const base::Version& component_version) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  UMA_HISTOGRAM_SPARSE_SLOWLY(
      kAssetsLoadStatus,
      EncodeVersionStatus(component_version, static_cast<int>(status)));
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
