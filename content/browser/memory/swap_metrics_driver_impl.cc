// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/swap_metrics_driver_impl.h"

#include <memory>

#include "base/macros.h"
#include "base/time/time.h"

namespace content {

SwapMetricsDriverImpl::Delegate::Delegate() = default;

SwapMetricsDriverImpl::Delegate::~Delegate() = default;

SwapMetricsDriverImpl::SwapMetricsDriverImpl(
    std::unique_ptr<Delegate> delegate,
    const base::TimeDelta update_interval)
    : delegate_(std::move(delegate)), update_interval_(update_interval) {
  DCHECK(delegate_);
}

SwapMetricsDriverImpl::~SwapMetricsDriverImpl() = default;

SwapMetricsDriver::SwapMetricsUpdateResult
SwapMetricsDriverImpl::InitializeMetrics() {
  last_ticks_ = base::TimeTicks();
  return UpdateMetrics();
}

bool SwapMetricsDriverImpl::IsRunning() {
  return timer_.IsRunning();
}

void SwapMetricsDriverImpl::PeriodicUpdateMetrics() {
  SwapMetricsDriver::SwapMetricsUpdateResult result = UpdateMetricsImpl();
  if (result !=
      SwapMetricsDriver::SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess)
    Stop();
}

SwapMetricsDriver::SwapMetricsUpdateResult SwapMetricsDriverImpl::Start() {
  DCHECK(update_interval_.InSeconds() > 0);

  SwapMetricsDriver::SwapMetricsUpdateResult result = InitializeMetrics();
  if (result !=
      SwapMetricsDriver::SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess)
    return result;

  timer_.Start(FROM_HERE, update_interval_, this,
               &SwapMetricsDriverImpl::PeriodicUpdateMetrics);
  return SwapMetricsDriver::SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess;
}

void SwapMetricsDriverImpl::Stop() {
  timer_.Stop();
}

SwapMetricsDriver::SwapMetricsUpdateResult
SwapMetricsDriverImpl::UpdateMetrics() {
  DCHECK(!IsRunning());
  auto result = UpdateMetricsImpl();
  if (result ==
      SwapMetricsDriver::SwapMetricsUpdateResult::kSwapMetricsUpdateFailed)
    delegate_->OnUpdateMetricsFailed();
  return result;
}

SwapMetricsDriver::SwapMetricsUpdateResult
SwapMetricsDriverImpl::UpdateMetricsImpl() {
  base::TimeTicks now = base::TimeTicks::Now();
  base::TimeDelta interval =
      last_ticks_.is_null() ? base::TimeDelta() : now - last_ticks_;

  SwapMetricsDriver::SwapMetricsUpdateResult result =
      UpdateMetricsInternal(interval);
  if (result !=
      SwapMetricsDriver::SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess)
    return result;

  last_ticks_ = now;
  return SwapMetricsDriver::SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess;
}

}  // namespace content
