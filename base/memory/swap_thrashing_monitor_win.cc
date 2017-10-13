// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/swap_thrashing_monitor.h"

#include <windows.h>

#include "base/bind.h"
#include "base/logging.h"

namespace base {
namespace {

using SwapThrashingLevel = SwapThrashingMonitor::SwapThrashingLevel;

}  // namespace

const int SwapThrashingMonitor::kPollingIntervalMs = 3000;

SwapThrashingMonitor::SwapThrashingMonitor()
    : current_swap_thrashing_level_(
          SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE) {
  StartObserving();
}

SwapThrashingMonitor::~SwapThrashingMonitor() {}

SwapThrashingLevel SwapThrashingMonitor::GetCurrentSwapThrashingLevel() {
  return SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;
}

void SwapThrashingMonitor::CheckSwapThrashingPressureAndRecordStatistics() {
  // SwapThrashingLevel old_thrashing_level = current_swap_thrashing_level_;
  current_swap_thrashing_level_ = CalculateCurrentSwapThrashingLevel();

  // TODO(sebmarchand): report the transition changes!
}

SwapThrashingLevel SwapThrashingMonitor::CalculateCurrentSwapThrashingLevel() {
  return SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;
}

void SwapThrashingMonitor::StartObserving() {
  MaybeStartObserving();
}

void SwapThrashingMonitor::MaybeStartObserving() {
  // TODO(sebmarchand): Determine if the system is using on-disk swap, abort if
  // it isn't as there won't be any swap-paging to observe.
  timer_.Start(
      FROM_HERE, TimeDelta::FromMilliseconds(kPollingIntervalMs),
      Bind(&SwapThrashingMonitor::CheckSwapThrashingPressureAndRecordStatistics,
           base::Unretained(this)));
}

}  // namespace base
