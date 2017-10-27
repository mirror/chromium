// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// SwapThrashingMonitor defines a state-based interface for a swap thrashing
// monitor. Thrashing is continuous swap activity, caused by processes needing
// to touch more pages than fit in physical memory, over a given period of time.
//
// The systems interested in observing these signals should query this monitor
// directly, it doesn't offer a notification API. Handling these signals should
// be done carefully in order to not aggravate the problem, in its initial
// implementation this monitor is meant to be used to measure the impact of
// thrashing on the core speed metrics.
//
// This monitor implements the following state machine:
//
//       ┎─────────────────────────┐
//       ┃SWAP_THRASHING_LEVEL_NONE┃
//       ┖─────────────────────────┚
//                   ▲
//                   |
//                   ▼
//    ┎──────────────────────────────┒
//    ┃SWAP_THRASHING_LEVEL_SUSPECTED┃
//    ┖──────────────────────────────┚
//                   ▲
//                   |
//                   ▼
//    ┎──────────────────────────────┒
//    ┃SWAP_THRASHING_LEVEL_CONFIRMED┃
//    ┖──────────────────────────────┚
//
// SWAP_THRASHING_LEVEL_NONE is the initial level, it means that there's no
// thrashing happening on the system or that it is undetermined. The monitor
// should then make a series of observations and when a sustained thrashing
// activity is observed it'll enter into the SWAP_THRASHING_LEVEL_SUSPECTED
// state.
//
// If the system continues observing a sustained thrashing activity on the
// system over a longer period of time then it'll enter into the
// SWAP_THRASHING_LEVEL_CONFIRMED state to indicate that the system is now in
// a confirmed swap-thrashing state, otherwise it'll go back to the
// SWAP_THRASHING_LEVEL_NONE state if there's no swap-thrashing observed over
// another series of observations.
//
// From the SWAP_THRASHING_LEVEL_CONFIRMED level the monitor can only go back to
// the SWAP_THRASHING_LEVEL_SUSPECTED state if there's no swap-thrashing
// observed.
//
// This monitor is responsible for initiating the observation of the
// swap-thrashing signals and it also enforce the state transition logic, the
// actual swap-thrashing observation logic should be done in a platform-specific
// implementation of the SwapThrashingDelegate class.

#ifndef CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_H_
#define CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_H_

#include "base/base_export.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "chrome/browser/memory/swap_thrashing_level.h"
#include "chrome/browser/memory/swap_thrashing_monitor_delegate.h"

namespace memory {

// Class for monitoring the swap-thrashing activity on the system.
//
// It is meant to be used as a singleton by calling the SetInstance method,
// e.g.:
//     SwapThrashingMonitor::SetInstance(
//         std::make_unique<SwapThrashingMonitor>());
//
// Then the current thrashing level can be obtained by calling
// SwapThrashingMonitor::GetInstance()->GetCurrentSwapThrashingLevel();
class SwapThrashingMonitor {
 public:
  // Constructor/Destructor.
  SwapThrashingMonitor();
  virtual ~SwapThrashingMonitor();

  // Sets the |SwapThrashingMonitor| global instance.
  static void SetInstance(std::unique_ptr<SwapThrashingMonitor> instance);

  // Returns the |SwapThrashingMonitor| global instance.
  static SwapThrashingMonitor* GetInstance();

  // Returns the currently observed swap-thrashing pressure.
  SwapThrashingLevel GetCurrentSwapThrashingLevel();

 protected:
  // Checks the swap-thrashing pressure, storing the current level, emitting
  // metrics as necessary if the pressure changes.
  void CheckSwapThrashingPressureAndRecordStatistics();

  // Starts observing the thrashing activity on the system.
  void StartObserving();

 private:
  // The delegate responsible for measuring the swap-thrashing activity.
  std::unique_ptr<SwapThrashingMonitorDelegate> delegate_;

  // The current swap-thrashing level.
  SwapThrashingLevel current_swap_thrashing_level_;

  // A periodic timer to check for swap-thrashing activity changes.
  base::RepeatingTimer timer_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SwapThrashingMonitor);
};

}  // namespace memory

#endif  // CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_H_
