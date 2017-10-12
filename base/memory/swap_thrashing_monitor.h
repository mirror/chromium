// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MEMORY_SWAP_THRASHING_MONITOR_H_
#define BASE_MEMORY_SWAP_THRASHING_MONITOR_H_

#include "base/base_export.h"
#include "base/macros.h"
#include "base/memory/swap_thrashing_listener.h"
#include "base/timer/timer.h"
#include "build/build_config.h"

namespace base {

class BASE_EXPORT SwapThrashingMonitor {
 public:
  using SwapThrashingLevel = SwapThrashingListener::SwapThrashingLevel;

  // The polling interval, in milliseconds.
  static const int kPollingIntervalMs;

  SwapThrashingMonitor();
  ~SwapThrashingMonitor();

  // Sets the |TabStatsTracker| global instance.
  static void SetInstance(std::unique_ptr<SwapThrashingMonitor> instance);

  // Returns the |SwapThrashingMonitor| global instance.
  static SwapThrashingMonitor* GetInstance();

  // Returns the currently observed swap-thrashing pressure.
  SwapThrashingLevel GetCurrentSwapThrashingLevel();

 protected:
  // Checks the swap-thrashing pressure, storing the current level, emitting
  // signals as necessary if the pressure changes.
  void CheckSwapThrashingPressure();

  // Calculates the current instantaneous swap-thrashing level.
  SwapThrashingLevel CalculateCurrentSwapThrashingLevel();

#if defined(OS_WIN)
  void MaybeStartObserving();
#endif

  // A periodic timer to check for swap-thrashing activity changes.
  base::RepeatingTimer timer_;

 private:
  // The current swap-thrashing level.
  SwapThrashingLevel current_swap_thrashing_level_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SwapThrashingMonitor);
};

}  // namespace base

#endif  // BASE_MEMORY_SWAP_THRASHING_MONITOR_H_
