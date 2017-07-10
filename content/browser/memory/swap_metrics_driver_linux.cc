// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/swap_metrics_driver_linux.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/process/process_metrics.h"
#include "content/browser/memory/swap_metrics_driver.h"
#include "content/browser/memory/swap_metrics_observer.h"

namespace content {

namespace {

bool HasSwap() {
  base::SystemMemoryInfoKB memory_info;
  if (!base::GetSystemMemoryInfo(&memory_info))
    return false;
  return memory_info.swap_total > 0;
}

}  // namespace

// static
std::unique_ptr<SwapMetricsDriver> SwapMetricsDriver::Create(
    std::unique_ptr<SwapMetricsObserver> observer,
    const int interval_in_seconds) {
  return HasSwap()
             ? base::WrapUnique<SwapMetricsDriver>(new SwapMetricsDriverLinux(
                   std::move(observer), interval_in_seconds))
             : std::unique_ptr<SwapMetricsDriver>();
}

SwapMetricsDriverLinux::SwapMetricsDriverLinux(
    std::unique_ptr<SwapMetricsObserver> observer,
    const int interval_in_seconds)
    : SwapMetricsDriver(std::move(observer), interval_in_seconds) {}

SwapMetricsDriverLinux::~SwapMetricsDriverLinux() = default;

SwapMetricsUpdateResult SwapMetricsDriverLinux::UpdateMetricsInternal(
    base::TimeDelta interval) {
  base::SystemMemoryInfoKB memory_info;
  if (!base::GetSystemMemoryInfo(&memory_info)) {
    return SwapMetricsUpdateResult::kSwapMetricsUpdateFailed;
  }

  double in_counts = memory_info.pswpin - last_pswpin_;
  double out_counts = memory_info.pswpout - last_pswpout_;
  last_pswpin_ = memory_info.pswpin;
  last_pswpout_ = memory_info.pswpout;

  if (interval.is_zero())
    return SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess;

  observer_->SwapsInPerSecond(in_counts / interval.InSecondsF(), interval);
  observer_->SwapsOutPerSecond(out_counts / interval.InSecondsF(), interval);

  return SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess;
}

}  // namespace content
