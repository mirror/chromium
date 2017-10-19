// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MEMORY_SWAP_THRASHING_DELEGATE_WIN_H_
#define BASE_MEMORY_SWAP_THRASHING_DELEGATE_WIN_H_

#include "base/sequence_checker.h"
#include "base/time/time.h"

namespace base {

class SwapThrashingMonitorDelegateWin {
 public:
  // The different swap thrashing level states.
  enum SwapThrashingLevel {
    // No evidence of swap-thrashing.
    SWAP_THRASHING_LEVEL_NONE,

    // There's been evidence of swap-thrashing.
    SWAP_THRASHING_LEVEL_SUSPECTED,

    // Swap-thrashing is confirmed.
    SWAP_THRASHING_LEVEL_CONFIRMED,
  };

  // Constructor/Destructor.
  SwapThrashingMonitorDelegateWin();
  ~SwapThrashingMonitorDelegateWin();

  // Calculates the swap-thrashing level over the interval between now and the
  // last time this function was called (of since the creation of this
  // delegate).
  SwapThrashingLevel CalculateCurrentSwapThrashingLevel();

 private:
  class PageFaultDetectionWindow {
   public:
    PageFaultDetectionWindow();
    PageFaultDetectionWindow(int window_length_ms, uint64_t page_fault_count);
    ~PageFaultDetectionWindow();

    void ResetWindow(int window_length_ms, uint64_t page_fault_count);

    void AddSample(uint64_t page_fault_observation, Time observation_timestamp);

    bool IsReady() const;

    void InvalidWindow();
    double average() const { return average_; }

   private:
    int window_length_ms_;
    uint64_t initial_page_fault_count_;
    Time window_beginning_;
    double average_;
  };

  bool MeasureHardPageFaultRate();

  // The detection window to detect the changes to a higher pressure state.
  PageFaultDetectionWindow swap_thrashing_observation_window_;

  // The cooldown detection window to detect the changes to a lower pressure
  // state.
  PageFaultDetectionWindow swap_thrashing_cooldown_window_;

  // The previous hard page-fault observation, used to compute the deltas.
  uint64_t previous_observation_;

  // The timestamp of the previous observation of the hard page-fault value.
  Time previous_observation_timestamp_;

  // The most recent swap-thrashing level measurement.
  SwapThrashingLevel last_thrashing_state_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SwapThrashingMonitorDelegateWin);
};

}  // namespace base

#endif  // BASE_MEMORY_SWAP_THRASHING_DELEGATE_WIN_H_
