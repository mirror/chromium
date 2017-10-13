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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return current_swap_thrashing_level_;
}

void SwapThrashingMonitor::CheckSwapThrashingPressureAndRecordStatistics() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(sebmarchand): Implement this function.
  NOTIMPLEMENTED();
}

SwapThrashingLevel SwapThrashingMonitor::CalculateCurrentSwapThrashingLevel() {
  // TODO(sebmarchand): Implement this function.
  NOTIMPLEMENTED();
  return SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;
}

void SwapThrashingMonitor::StartObserving() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  MaybeStartObserving();
}

void SwapThrashingMonitor::MaybeStartObserving() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(sebmarchand): Determine if the system is using on-disk swap, abort if
  // it isn't as there won't be any swap-paging to observe (on-disk swap could
  // later become available if the user turn it on but this case is rare that
  // it's safe to ignore it).
  timer_.Start(
      FROM_HERE, TimeDelta::FromMilliseconds(kPollingIntervalMs),
      Bind(&SwapThrashingMonitor::CheckSwapThrashingPressureAndRecordStatistics,
           base::Unretained(this)));
}

}  // namespace base
