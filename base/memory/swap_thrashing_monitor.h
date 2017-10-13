// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Define a state-based interface for a swap thrashing monitor. Thrashing is
// continuous swap activity, caused by processes needing to touch more pages
// than fit in physical memory, over a given period of time.
//
// The platform-specific implementation of this interface should implement the
// following state machine:
//
//    +-------------------------+  (T1)  +------------------------------+
//    |SWAP_THRASHING_LEVEL_NONE|◄------►|SWAP_THRASHING_LEVEL_SUSPECTED|
//    +-------------------------+        +------------------------------+
//                                               ▲
//                                               |
//                                               | (T2)
//                                               |
//                                               ▼
//                   +------------------------------+
//                   |SWAP_THRASHING_LEVEL_CONFIRMED|
//                   +------------------------------+
//
// SWAP_THRASHING_LEVEL_NONE is the initial level, it means that there's no
// thrashing happening on the system or that it is undetermined. The monitor
// should then make a series of observations and when a sustained thrashing
// activity is observed over a period of time T1 it'll enter into the
// SWAP_THRASHING_LEVEL_SUSPECTED state.
//
// If the system continues observing a sustained thrashing activity on the
// system over a second period of time T2 then it'll enter into the
// SWAP_THRASHING_LEVEL_CONFIRMED state to indicate that the system is now in
// a confirmed swap-thrashing state, otherwise it'll go back to the
// SWAP_THRASHING_LEVEL_NONE state if there's no swap-thrashing observed during
// another interval of time T1' (T1 with an hysteresis).
//
// From the SWAP_THRASHING_LEVEL_CONFIRMED level the monitor can go back to the
// SWAP_THRASHING_LEVEL_NONE or to the SWAP_THRASHING_LEVEL_SUSPECTED depending
// on the volume of swap-thrashing observed.

#ifndef BASE_MEMORY_SWAP_THRASHING_MONITOR_H_
#define BASE_MEMORY_SWAP_THRASHING_MONITOR_H_

#include "base/base_export.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "build/build_config.h"

namespace base {

// Class for monitoring the swap-thrashing activity on the system.
//
// It is meant to be used as a singleton by calling the SetInstance method,
// e.g.:
//     SwapThrashingMonitor::SetInstance(
//         std::make_unique<SwapThrashingMonitor>());
//
// Then the current thrashing level can be obtained by calling
// SwapThrashingMonitor::GetInstance()->GetCurrentSwapThrashingLevel();
class BASE_EXPORT SwapThrashingMonitor {
 public:
  enum SwapThrashingLevel {
    // No evidence of swap-thrashing.
    SWAP_THRASHING_LEVEL_NONE,

    // There's been evidence of swap-thrashing.
    SWAP_THRASHING_LEVEL_SUSPECTED,

    // Swap-thrashing is confirmed.
    SWAP_THRASHING_LEVEL_CONFIRMED,
  };

  // The polling interval, in milliseconds.
  static const int kPollingIntervalMs;

  // The period of time during which swap-thrashing has to be observed in order
  // to switch from the SWAP_THRASHING_LEVEL_NONE to the
  // SWAP_THRASHING_LEVEL_SUSPECTED level, in milliseconds.
  static const int kNoneToSuspectedIntervalMs;

  // The period of time during which swap-thrashing should not be observed in
  // order to switch from the SWAP_THRASHING_LEVEL_SUSPECTED to the
  // SWAP_THRASHING_LEVEL_NONE level, in milliseconds.
  static const int kSuspectedIntervalCoolDownMs;

  // The period of time during which swap-thrashing has to be observed while
  // being in the SWAP_THRASHING_LEVEL_SUSPECTED state in order to switch to the
  // SWAP_THRASHING_LEVEL_CONFIRMED level, in milliseconds.
  static const int kSuspectedToConfirmedIntervalMs;

  // The period of time during which swap-thrashing should not be observed in
  // order to switch from the SWAP_THRASHING_LEVEL_CONFIRMED to the
  // SWAP_THRASHING_LEVEL_SUSPECTED level, in milliseconds.
  static const int kConfirmedCoolDownMs;

  // Constructor/Destructor.
  //
  // Requires a platform specific implementation.
  SwapThrashingMonitor();
  ~SwapThrashingMonitor();

  // Sets the |TabStatsTracker| global instance.
  static void SetInstance(std::unique_ptr<SwapThrashingMonitor> instance);

  // Returns the |SwapThrashingMonitor| global instance.
  static SwapThrashingMonitor* GetInstance();

  // Returns the currently observed swap-thrashing pressure.
  //
  // Requires a platform specific implementation.
  SwapThrashingLevel GetCurrentSwapThrashingLevel();

 private:
  // Checks the swap-thrashing pressure, storing the current level, emitting
  // signals as necessary if the pressure changes.
  //
  // Requires a platform specific implementation.
  void CheckSwapThrashingPressureAndRecordStatistics();

  // Calculates the current instantaneous swap-thrashing level.
  //
  // Requires a platform specific implementation.
  SwapThrashingLevel CalculateCurrentSwapThrashingLevel();

  // Starts observing the thrashing activity on the system.
  //
  // Requires a platform specific implementation.
  void StartObserving();

#if defined(OS_WIN)
  // Starts observing the thrashing activity on the system if on-disk swap is
  // enabled.
  void MaybeStartObserving();

  // The current swap-thrashing level.
  SwapThrashingLevel current_swap_thrashing_level_;
#endif

  // A periodic timer to check for swap-thrashing activity changes.
  base::RepeatingTimer timer_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SwapThrashingMonitor);
};

}  // namespace base

#endif  // BASE_MEMORY_SWAP_THRASHING_MONITOR_H_
