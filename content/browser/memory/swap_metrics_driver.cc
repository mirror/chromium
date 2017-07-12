// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/swap_metrics_driver.h"

#include <memory>

#include "base/macros.h"
#include "content/browser/memory/swap_metrics_observer.h"

namespace content {

SwapMetricsDriver::SwapMetricsDriver(
    std::unique_ptr<SwapMetricsObserver> observer,
    const int interval_in_seconds)
    : observer_(std::move(observer)),
      update_interval_(base::TimeDelta::FromSeconds(interval_in_seconds)) {
  DCHECK(observer_);
}

SwapMetricsDriver::~SwapMetricsDriver() {}

SwapMetricsUpdateResult SwapMetricsDriver::InitializeMetrics() {
  last_ticks_ = base::TimeTicks();
  return UpdateMetrics();
}

void SwapMetricsDriver::PeriodicUpdateMetrics() {
  SwapMetricsUpdateResult result = UpdateMetricsImpl();
  if (result != SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess)
    Stop();
}

SwapMetricsUpdateResult SwapMetricsDriver::Start() {
  DCHECK(update_interval_.InSeconds() > 0);

  SwapMetricsUpdateResult result = InitializeMetrics();
  if (result != SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess)
    return result;

  timer_.Start(FROM_HERE, update_interval_, this,
               &SwapMetricsDriver::PeriodicUpdateMetrics);
  return SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess;
}

void SwapMetricsDriver::Stop() {
  timer_.Stop();
}

SwapMetricsUpdateResult SwapMetricsDriver::UpdateMetrics() {
  DCHECK(!timer_.IsRunning());
  return UpdateMetricsImpl();
}

SwapMetricsUpdateResult SwapMetricsDriver::UpdateMetricsImpl() {
  base::TimeTicks now = base::TimeTicks::Now();
  base::TimeDelta interval =
      last_ticks_.is_null() ? base::TimeDelta() : now - last_ticks_;

  SwapMetricsUpdateResult result = UpdateMetricsInternal(interval);
  if (result != SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess)
    return result;

  last_ticks_ = now;
  return SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess;
}

}  // namespace content
