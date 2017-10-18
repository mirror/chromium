// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MEMORY_SWAP_THRASHING_DELEGATE_H_
#define BASE_MEMORY_SWAP_THRASHING_DELEGATE_H_

#include "base/base_export.h"
#include "base/containers/circular_deque.h"
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

    // Indicates that the current swap-thrashing level is invalid and that
    // there's some logic issue in the state transition code.
    SWAP_THRASHING_LEVEL_INVALID,
  };

  // Constructor/Destructor.
  SwapThrashingDelegate();
  virtual ~SwapThrashingDelegate();

  // Returns the interval at which the swap-thrashing level should be measured.
  virtual int GetPollingIntervalMs();

  // Calculates the current instantaneous swap-thrashing level.
  //
  // The current level should be based on the |previous_state| level and should
  // respect the state transition logic allowed by the SwapThrashingMonitor
  // class.
  virtual SwapThrashingLevel CalculateCurrentSwapThrashingLevel(
      SwapThrashingDelegate::SwapThrashingLevel previous_state);

 private:
  class PageFaultDetectionWindow {
   public:
    explicit PageFaultDetectionWindow(size_t window_size);
    ~PageFaultDetectionWindow();

    void ResetWindow(size_t new_window_size);

    void AddSample(uint64_t sample);

    bool IsReady() const;
    double GetAverage() const;

   private:
    size_t window_size_;
    base::circular_deque<uint64_t> page_fault_counts_;
    double average_;
  };

  // The detection window to detect the changes to a higher pressure state.
  PageFaultDetectionWindow swap_thrashing_observation_window_;

  // The cooldown detection window to detect the changes to a lower pressure
  // state.
  PageFaultDetectionWindow swap_thrashing_cooldown_window_;

  // The previous hard-page fault observation, used to compute the deltas.
  uint64_t previous_observation_;

  // Indicates if |previous_observation_| is valid.
  bool previous_observation_is_valid_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SwapThrashingDelegate);
};

}  // namespace base

#endif  // BASE_MEMORY_SWAP_THRASHING_DELEGATE_H_
