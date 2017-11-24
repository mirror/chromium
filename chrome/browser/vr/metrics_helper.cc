// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/metrics_helper.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"

namespace vr {

namespace {

static constexpr char kStatusVr[] = "VR.AssetsComponent.Status.OnEnter.VR";
static constexpr char kStatusVrBrowsing[] =
    "VR.AssetsComponent.Status.OnEnter.VRBrowsing";
static constexpr char kStatusWebVr[] =
    "VR.AssetsComponent.Status.OnEnter.WebVR";
static constexpr char kLatencyVr[] =
    "VR.AssetsComponent.ReadyLatency.OnEnter.VR";
static constexpr char kLatencyVrBrowsing[] =
    "VR.AssetsComponent.ReadyLatency.OnEnter.VRBrowsing";
static constexpr char kLatencyWebVr[] =
    "VR.AssetsComponent.ReadyLatency.OnEnter.WebVR";

// Ensure that this stays in sync with VRAssetsComponentEnterStatus in
// enums.xml.
enum class AssetsComponentEnterStatus : int {
  kReady = 0,
  kUnreadyOther,

  // This must be last.
  kCount,
};

static const char* GetStatusHistogramName(Mode mode) {
  switch (mode) {
    case Mode::kVr:
      return kStatusVr;
    case Mode::kVrBrowsing:
      return kStatusVrBrowsing;
    case Mode::kWebVr:
      return kStatusWebVr;
    default:
      NOTREACHED();
      return nullptr;
  }
}

static const char* GetLatencyHistogramName(Mode mode) {
  switch (mode) {
    case Mode::kVr:
      return kLatencyVr;
    case Mode::kVrBrowsing:
      return kLatencyVrBrowsing;
    case Mode::kWebVr:
      return kLatencyWebVr;
    default:
      NOTREACHED();
      return nullptr;
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
  LogWaitLatencyIfWaited<Mode::kVr>(now);
  LogWaitLatencyIfWaited<Mode::kVrBrowsing>(now);
  LogWaitLatencyIfWaited<Mode::kWebVr>(now);
}

void MetricsHelper::OnEnter(Mode mode) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  switch (mode) {
    case Mode::kVr:
      OnEnter<Mode::kVr>();
      return;
    case Mode::kVrBrowsing:
      OnEnter<Mode::kVrBrowsing>();
      return;
    case Mode::kWebVr:
      OnEnter<Mode::kWebVr>();
      return;
    default:
      NOTREACHED();
      return;
  }
}

template <Mode ModeName>
void MetricsHelper::OnEnter() {
  auto& enter_time = GetEnterTime(ModeName);
  if (enter_time) {
    // While we are stopping the time between entering and component readiness
    // we pretend the user is waiting on the block screen. Do not report further
    // UMA metrics.
    return;
  }
  UMA_HISTOGRAM_ENUMERATION(GetStatusHistogramName(ModeName),
                            component_ready_
                                ? AssetsComponentEnterStatus::kReady
                                : AssetsComponentEnterStatus::kUnreadyOther,
                            AssetsComponentEnterStatus::kCount);
  if (!component_ready_) {
    enter_time = base::Time::Now();
  }
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
      NOTREACHED();
      return enter_vr_time_;
  }
}

template <Mode ModeName>
void MetricsHelper::LogWaitLatencyIfWaited(const base::Time& now) {
  auto& enter_time = GetEnterTime(ModeName);
  if (enter_time) {
    auto latency = now - *enter_time;
    UMA_HISTOGRAM_CUSTOM_TIMES(GetLatencyHistogramName(ModeName), latency,
                               base::TimeDelta::FromMilliseconds(500),
                               base::TimeDelta::FromHours(5), 100);
    enter_time = base::nullopt;
  }
}

}  // namespace vr
