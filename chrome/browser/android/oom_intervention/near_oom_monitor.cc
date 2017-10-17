// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/oom_intervention/near_oom_monitor.h"

#include "base/process/process_metrics.h"

namespace {

const int64_t kDefaultSwapFreeThreshold = 150000;
constexpr base::TimeDelta kDefaultMonitoringDelta =
    base::TimeDelta::FromSeconds(1);

}  // namespace

NearOomMonitor* NearOomMonitor::GetInstance() {
  static NearOomMonitor* instance = new NearOomMonitor();
  return instance;
}

NearOomMonitor::NearOomMonitor()
    : monitoring_interval_(kDefaultMonitoringDelta),
      swapfree_threshold_(kDefaultSwapFreeThreshold),
      observers_(new base::ObserverListThreadSafe<Observer>()) {}

NearOomMonitor::~NearOomMonitor() = default;

void NearOomMonitor::Start() {
  timer_.Start(FROM_HERE, monitoring_interval_, this, &NearOomMonitor::Check);
}

void NearOomMonitor::Register(Observer* observer) {
  observers_->AddObserver(observer);
}

void NearOomMonitor::Unregister(Observer* observer) {
  observers_->RemoveObserver(observer);
}

void NearOomMonitor::Check() {
  base::SystemMemoryInfoKB memory_info;
  base::GetSystemMemoryInfo(&memory_info);
  if (memory_info.swap_free <= swapfree_threshold_) {
    LOG(ERROR) << "Detected Near-OOM";
    observers_->Notify(FROM_HERE, &NearOomMonitor::Observer::OnNearOomDetected);
  }
}
