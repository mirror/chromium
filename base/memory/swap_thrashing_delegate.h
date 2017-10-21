// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The delegate responsible for measuring the current swap thrashing level.
//
// The platforms interested in observing these signals should provide a
// platform-specific implementation of this class.

#ifndef BASE_MEMORY_SWAP_THRASHING_DELEGATE_H_
#define BASE_MEMORY_SWAP_THRASHING_DELEGATE_H_

#include "base/base_export.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "build/build_config.h"

namespace base {

// A dummy SwapThrashingMonitorDelegate that doesn't do anything.
class BASE_EXPORT SwapThrashingMonitorDelegateInterface {
 public:
  // The different swap thrashing level states. See
  // base/memory/swap_thrashing_monitor.h for more details on how the system
  // can transition from one state to another.
  enum SwapThrashingLevel {
    // No evidence of swap-thrashing.
    SWAP_THRASHING_LEVEL_NONE,

    // There's been evidence of swap-thrashing.
    SWAP_THRASHING_LEVEL_SUSPECTED,

    // Swap-thrashing is confirmed.
    SWAP_THRASHING_LEVEL_CONFIRMED,

    // Indicates that the current swap-thrashing level is invalid and that
    // there's some logic issue in the state transition code.
    SWAP_THRASHING_LEVEL_INVALID,
  };

  // Constructor/Destructor.
  SwapThrashingMonitorDelegateInterface();
  virtual ~SwapThrashingMonitorDelegateInterface();

  // Returns the interval at which the swap-thrashing level should be measured.
  virtual int GetPollingIntervalMs();

  // Estimates the variation in swap-thrashing level between now and the last
  // time this function was called.
  //
  // The current level should be based on the |previous_state| level and should
  // respect the state transition logic allowed by the SwapThrashingMonitor
  // class.
  virtual SwapThrashingLevel CalculateCurrentSwapThrashingLevel(
      SwapThrashingMonitorDelegateInterface::SwapThrashingLevel previous_state);

 private:
  DISALLOW_COPY_AND_ASSIGN(SwapThrashingMonitorDelegateInterface);
};

}  // namespace base

#endif  // BASE_MEMORY_SWAP_THRASHING_DELEGATE_H_
