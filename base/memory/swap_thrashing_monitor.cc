// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/swap_thrashing_monitor.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"

namespace base {
namespace {

// The global SwapThrashingMonitor instance.
SwapThrashingMonitor* g_instance = nullptr;

}  // namespace

// static
void SwapThrashingMonitor::SetInstance(
    std::unique_ptr<SwapThrashingMonitor> instance) {
  DCHECK_EQ(nullptr, g_instance);
  g_instance = instance.release();
}

SwapThrashingMonitor* SwapThrashingMonitor::GetInstance() {
  return g_instance;
}

SwapThrashingMonitor::SwapThrashingMonitor()
    : delegate_(base::MakeUnique<base::SwapThrashingDelegate>()) {
  StartObserving();
}

SwapThrashingMonitor::SwapThrashingMonitor(
    std::unique_ptr<SwapThrashingDelegate> delegate)
    : delegate_(std::move(delegate)) {
  StartObserving();
}

SwapThrashingMonitor::~SwapThrashingMonitor() {}

SwapThrashingMonitor::SwapThrashingLevel
SwapThrashingMonitor::GetCurrentSwapThrashingLevel() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return current_swap_thrashing_level_;
}

void SwapThrashingMonitor::CheckSwapThrashingPressureAndRecordStatistics() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  SwapThrashingLevel swap_thrashing_pressure =
      delegate_->CalculateCurrentSwapThrashingLevel(
          current_swap_thrashing_level_);

  if (swap_thrashing_pressure == current_swap_thrashing_level_)
    return;

  // TODO(sebmarchand): Handle the state transitions and report some UMA
  // metrics.

  NOTIMPLEMENTED();
}

void SwapThrashingMonitor::StartObserving() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(sebmarchand): Determine if the system is using on-disk swap, abort if
  // it isn't as there won't be any swap-paging to observe (on-disk swap could
  // later become available if the user turn it on but this case is rare that
  // it's safe to ignore it).
  int timer_interval = delegate_->GetPollingIntervalMs();
  if (timer_interval != 0) {
    timer_.Start(FROM_HERE, TimeDelta::FromMilliseconds(timer_interval),
                 Bind(&SwapThrashingMonitor::
                          CheckSwapThrashingPressureAndRecordStatistics,
                      base::Unretained(this)));
  }
}

}  // namespace base
