// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The delegate responsible for measuring the current swap thrashing level.

#ifndef BASE_MEMORY_SWAP_THRASHING_DELEGATE_H_
#define BASE_MEMORY_SWAP_THRASHING_DELEGATE_H_

#include "base/base_export.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "build/build_config.h"

namespace base {

class BASE_EXPORT SwapThrashingDelegate {
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
  };

  // Constructor/Destructor.
  SwapThrashingDelegate();
  virtual ~SwapThrashingDelegate();

  virtual int GetPollingIntervalMs();

  // Calculates the current instantaneous swap-thrashing level.
  virtual SwapThrashingLevel CalculateCurrentSwapThrashingLevel(
      SwapThrashingDelegate::SwapThrashingLevel previous_state);

 private:
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

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SwapThrashingDelegate);
};

}  // namespace base

#endif  // BASE_MEMORY_SWAP_THRASHING_DELEGATE_H_
