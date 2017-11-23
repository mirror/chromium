// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/metrics_helper.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequence_checker.h"

namespace vr {

namespace {

static constexpr char kReadyVr[] = "VR.AssetsComponentReady.OnEnter.VR";
static constexpr char kReadyVrBrowsing[] =
    "VR.AssetsComponentReady.OnEnter.VRBrowsing";
static constexpr char kReadyWebVr[] = "VR.AssetsComponentReady.OnEnter.WebVR";
static constexpr char kLatencyVr[] =
    "VR.AssetsComponentReadyLatency.OnEnter.VR";
static constexpr char kLatencyVrBrowsing[] =
    "VR.AssetsComponentReadyLatency.OnEnter.VRBrowsing";
static constexpr char kLatencyWebVr[] =
    "VR.AssetsComponentReadyLatency.OnEnter.WebVR";

static const char* GetReadyHistogramName(Mode mode) {
  switch (mode) {
    case Mode::kVr:
      return kReadyVr;
    case Mode::kVrBrowsing:
      return kReadyVrBrowsing;
    case Mode::kWebVr:
      return kReadyWebVr;
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

MetricsHelper::MetricsHelper() = default;

MetricsHelper::~MetricsHelper() = default;

void MetricsHelper::OnComponentReady() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(*GetSequenceChecker());
  component_ready_ = true;
  auto now = base::Time::Now();
  LogWaitLatencyIfWaited<Mode::kVr>(now);
  LogWaitLatencyIfWaited<Mode::kVrBrowsing>(now);
  LogWaitLatencyIfWaited<Mode::kWebVr>(now);
}

void MetricsHelper::OnEnter(Mode mode) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(*GetSequenceChecker());
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
  UMA_HISTOGRAM_BOOLEAN(GetReadyHistogramName(ModeName), component_ready_);
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

base::SequenceChecker* MetricsHelper::GetSequenceChecker() {
  // If we instantiate sequence_checker_ in the constructor we have to call
  // OnComponentReady, etc. on the thread the constructor ran. That will not
  // always be the case and is not required for thread-safety. Instead, create
  // sequence_checker_ the first time want to make sequential calls a
  // requirement.
  if (!sequence_checker_) {
    sequence_checker_ = base::MakeUnique<base::SequenceChecker>();
  }
  return sequence_checker_.get();
}

}  // namespace vr
