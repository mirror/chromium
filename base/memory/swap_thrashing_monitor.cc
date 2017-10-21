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
    : delegate_(base::MakeUnique<base::SwapThrashingMonitorDelegate>()),
      current_swap_thrashing_level_(
          SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE) {
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

  // TODO(sebmarchand): Record some UMA histograms about the state transitions.

  switch (current_swap_thrashing_level_) {
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE: {
      if (swap_thrashing_pressure ==
          SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED) {
        current_swap_thrashing_level_ = swap_thrashing_pressure;
      } else {
        current_swap_thrashing_level_ =
            SwapThrashingLevel::SWAP_THRASHING_LEVEL_INVALID;
      }
      break;
    }
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED: {
      if (swap_thrashing_pressure ==
              SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE ||
          swap_thrashing_pressure ==
              SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED) {
        current_swap_thrashing_level_ = swap_thrashing_pressure;
      } else {
        current_swap_thrashing_level_ =
            SwapThrashingLevel::SWAP_THRASHING_LEVEL_INVALID;
      }
      break;
    }
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED: {
      if (swap_thrashing_pressure ==
          SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED) {
        current_swap_thrashing_level_ = swap_thrashing_pressure;
      } else {
        current_swap_thrashing_level_ =
            SwapThrashingLevel::SWAP_THRASHING_LEVEL_INVALID;
      }
      break;
    }
    default:
      break;
  }
  if (current_swap_thrashing_level_ ==
      SwapThrashingLevel::SWAP_THRASHING_LEVEL_INVALID) {
    LOG(ERROR) << "Invalid swap-thrashing state transition, stopping the "
               << "monitor.";
    if (timer_.IsRunning())
      timer_.Stop();
  }
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

// static
void SwapThrashingMonitor::ReleaseInstance() {
  DCHECK_NE(nullptr, g_instance);
  delete g_instance;
  g_instance = nullptr;
}

}  // namespace base
