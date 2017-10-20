// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// SwapThrashingMonitorDelegateWin defines a state-based swap thrashing
// detector on Windows. Thrashing is continuous swap activity, caused by
// processes needing to touch more pages than fit in physical memory, over a
// given period of time.
//
// The systems interested in observing these signals should ensure that the
// CalculateCurrentSwapThrashingLevel function of this detector gets called
// periodically.
//
// This detector implements the following state machine:
//
//    ┎─────────────────────────┐  (T1)  ┎──────────────────────────────┒
//    ┃SWAP_THRASHING_LEVEL_NONE┃◄------►┃SWAP_THRASHING_LEVEL_SUSPECTED┃
//    ┖─────────────────────────┚        ┖──────────────────────────────┚
//                                               ▲
//                                               |
//                                               | (T2)
//                                               |
//                                               ▼
//                   ┎──────────────────────────────┐
//                   ┃SWAP_THRASHING_LEVEL_CONFIRMED┃
//                   ┖──────────────────────────────┚
//
// SWAP_THRASHING_LEVEL_NONE is the initial level, it means that there's no
// thrashing happening on the system or that it is undetermined. The detector
// should regularly observe the hard page-fault counters and when a sustained
// thrashing activity is observed over a period of time T1 it'll enter into the
// SWAP_THRASHING_LEVEL_SUSPECTED state.
//
// If the system continues observing a sustained thrashing activity on the
// system over a second period of time T2 then it'll enter into the
// SWAP_THRASHING_LEVEL_CONFIRMED state to indicate that the system is now in
// a confirmed swap-thrashing state, otherwise it'll go back to the
// SWAP_THRASHING_LEVEL_NONE state if there's no swap-thrashing observed during
// another interval of time T1' (T1 with an hysteresis).
//
// From the SWAP_THRASHING_LEVEL_CONFIRMED level the detector can only go back
// to the SWAP_THRASHING_LEVEL_SUSPECTED state if there's no swap-thrashing
// observed during an interval of time T2' (T2 with an hysteresis).

#ifndef BASE_MEMORY_SWAP_THRASHING_DELEGATE_WIN_H_
#define BASE_MEMORY_SWAP_THRASHING_DELEGATE_WIN_H_

#include "base/sequence_checker.h"
#include "base/time/time.h"

namespace base {

class BASE_EXPORT SwapThrashingMonitorDelegateWin {
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
  virtual ~SwapThrashingMonitorDelegateWin();

  // Calculates the swap-thrashing level over the interval between now and the
  // last time this function was called (of since the creation of this
  // delegate).
  SwapThrashingLevel CalculateCurrentSwapThrashingLevel();

 protected:
  // The average hard page-fault rate (/sec) that we expect to see when
  // switching to a higher swap-thrashing level.
  // TODO(sebmarchand): Confirm that this value is appropriate.
  static const size_t kThrashSwappingPageFaultThreshold;

  // The average hard page-fault rate (/sec) that we use to go down to a lower
  // swap-thrashing level.
  // TODO(sebmarchand): Confirm that this value is appropriate.
  static const size_t kPageFaultCooldownThreshold;

  // A page-fault detection window, it is used to analyze the average hard
  // page-fault rate during a given period of time.
  class BASE_EXPORT PageFaultDetectionWindow {
   public:
    // Constructor,
    PageFaultDetectionWindow(int window_length_ms, uint64_t page_fault_count);
    virtual ~PageFaultDetectionWindow();

    // Should be called when a new hard-page fault observation is made. This
    // will check if the time since the initial observation exceeds the length
    // of this window and will compute the average hard page-fault rate during
    // this window if it's the case.
    virtual void OnObservation(uint64_t page_fault_observation,
                               Time observation_timestamp);

    // Check if the average hard page-fault rate is ready to be consulted.
    virtual bool IsReady() const;

    // Return the average hard page-fault rate during this observation window,
    // in hard page-fault / sec. Returns -1.0 if this value isn't available yet.
    double average_page_fault_rate() const { return average_page_fault_rate_; }

   protected:
    // The length of this observation window, in ms.
    int window_length_ms_;

    // The hard page-fault count recorded at the beginning of this observation
    // window.
    uint64_t initial_page_fault_count_;

    // The time at the creation of this observation window.
    Time window_beginning_;

    // The average hard page-fault rate, in hard page-fault / sec. Equal to -1
    // if this value isn't available yet.
    double average_page_fault_rate_;
  };

  // Get the global hard page-fault counter for all the Chrome processes.
  bool GetChromeHardPageFaultCounter();

  // Reset the observation/cooldown window to the value provided.
  //
  // These functions are virtual for unittesting purposes.
  virtual void ResetDetectionWindow(int window_length_ms,
                                    uint64_t page_fault_count);
  virtual void ResetCooldownWindow(int window_length_ms,
                                   uint64_t page_fault_count);

  // The detection window to detect the changes to a higher pressure state.
  std::unique_ptr<PageFaultDetectionWindow> swap_thrashing_detection_window_;

  // The cooldown detection window to detect the changes to a lower pressure
  // state.
  std::unique_ptr<PageFaultDetectionWindow> swap_thrashing_cooldown_window_;

  // The previous hard page-fault observation, used to compute the deltas.
  uint64_t previous_observation_;

  // The timestamp of the previous observation of the hard page-fault value.
  Time previous_observation_timestamp_;

  // The most recent swap-thrashing level measurement.
  SwapThrashingLevel last_thrashing_state_;

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SwapThrashingMonitorDelegateWin);
};

}  // namespace base

#endif  // BASE_MEMORY_SWAP_THRASHING_DELEGATE_WIN_H_
