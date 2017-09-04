// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/android/near_oom_monitor.h"

#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "content/renderer/render_thread_impl.h"

namespace content {

namespace {

constexpr base::TimeDelta kDefaultMonitoringDelta =
    base::TimeDelta::FromSeconds(1);
const int64_t kDefaultThreshold = 130 * 1024 * 1024;

}  // namespace

// static
std::unique_ptr<NearOomMonitor> NearOomMonitor::Create() {
  return base::MakeUnique<NearOomMonitor>();
}

NearOomMonitor::NearOomMonitor()
    : threshold_(kDefaultThreshold),
      monitoring_interval_(kDefaultMonitoringDelta),
      resumed_(false) {
  const std::string& value =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          "near-oom-threshold-bytes");
  if (!value.empty())
    base::StringToInt64(value, &threshold_);
}

NearOomMonitor::~NearOomMonitor() = default;

void NearOomMonitor::Start() {
  metrics_ = base::ProcessMetrics::CreateCurrentProcessMetrics();
  CHECK(metrics_);
  timer_.Start(FROM_HERE, monitoring_interval_, this, &NearOomMonitor::Check);
}

void NearOomMonitor::ResumeIfNeeded() {
  suspender_.reset();
  resumed_ = true;
  LOG(ERROR) << "Resumed pages";
}

void NearOomMonitor::Check() {
  size_t value = metrics_->GetWorkingSetSize();
  if (value >= threshold_ && !suspender_ && !resumed_) {
    suspender_.reset(new blink::WebPageSuspender);
    RenderThreadImpl::current()->GetRendererHost()->OnPagesSuspended();
  }
}

}  // namespace content
