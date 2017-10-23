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

#ifndef CHROME_BROWSER_MEMORY_SWAP_THRASHING_DELEGATE_WIN_H_
#define CHROME_BROWSER_MEMORY_SWAP_THRASHING_DELEGATE_WIN_H_

#include <list>

#include "base/sequence_checker.h"
#include "base/time/time.h"

namespace memory {

class SwapThrashingMonitorDelegateWin {
 public:
  // The different swap thrashing level states.
  enum class SwapThrashingLevel {
    // No evidence of swap-thrashing.
    SWAP_THRASHING_LEVEL_NONE,

    // There's been evidence of swap-thrashing.
    SWAP_THRASHING_LEVEL_SUSPECTED,

    // Swap-thrashing is confirmed.
    SWAP_THRASHING_LEVEL_CONFIRMED,
  };

  // A hard page-fault observation.
  struct PageFaultObservation {
    uint64_t page_fault_count;
    base::TimeTicks timestamp;
  };

  // Constructor/Destructor.
  SwapThrashingMonitorDelegateWin();
  virtual ~SwapThrashingMonitorDelegateWin();

  // Calculates the swap-thrashing level over the interval between now and the
  // last time this function was called (of since the creation of this
  // delegate).
  //
  // This function will run on the same thread where this detector has been
  // instantiated and will require to do a system call.
  //
  // TODO(sebmarchand): Check if this should be posted to a task runner and run
  // asynchronously.
  SwapThrashingLevel CalculateCurrentSwapThrashingLevel();

 protected:
  // The average hard page-fault rate (/sec) that we expect to see when
  // switching to a higher swap-thrashing level.
  // TODO(sebmarchand): Confirm that this value is appropriate.
  static const size_t kPageFaultEscalationThreshold;

  // The average hard page-fault rate (/sec) that we use to go down to a lower
  // swap-thrashing level.
  // TODO(sebmarchand): Confirm that this value is appropriate.
  static const size_t kPageFaultCooldownThreshold;

  // A page-fault detection window, it is used to analyze the average hard
  // page-fault rate during a given period of time.
  class PageFaultDetectionWindow {
   public:
    // Constructor,
    explicit PageFaultDetectionWindow(base::TimeDelta window_length);
    virtual ~PageFaultDetectionWindow();

    // Reset the window to the value provided.
    void Reset(base::TimeDelta window_length);

    // Should be called when a new hard-page fault observation is made. The
    // observation will be added to the list of observations and the one that
    // are not needed anymore will be removed.
    virtual void OnObservation(PageFaultObservation observation);

    // Computes the average hard page-fault rate during this observation window.
    //
    // Returns true if the window of time has expired and store the average
    // value in |average_rate|, returns false otherwise.
    virtual bool AveragePageFaultRate(double* average_rate) const;

    // Accessor.
    base::TimeDelta window_length() const { return window_length_; }

    // Return the most recent observation that has been recorded by this
    // detection window.
    const PageFaultObservation& MostRecentObservation() {
      DCHECK(!observations_.empty);
      return observations_.back();
    }

   protected:
    // The length of this observation window.
    base::TimeDelta window_length_;

    // The observation, in the order they have been received.
    std::list<PageFaultObservation> observations_;

   private:
    SEQUENCE_CHECKER(sequence_checker_);
  };

  // Get the global hard page-fault counter for all the Chrome processes.
  virtual bool GetChromeHardPageFaultCounter();

  // Reset the escalation/cooldown windows.
  //
  // These functions are virtual for unittesting purposes.
  virtual void ResetEscalationWindow(base::TimeDelta window_length);
  virtual void ResetCooldownWindow(base::TimeDelta window_length);

  // The escalation window to detect the changes to a higher thrashing state.
  std::unique_ptr<PageFaultDetectionWindow> swap_thrashing_escalation_window_;

  // The cooldown window to detect the changes to a lower thrashing state.
  std::unique_ptr<PageFaultDetectionWindow> swap_thrashing_cooldown_window_;

  // The most recent swap-thrashing level measurement.
  SwapThrashingLevel last_thrashing_state_;

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SwapThrashingMonitorDelegateWin);
};

}  // namespace memory

#endif  // CHROME_BROWSER_MEMORY_SWAP_THRASHING_DELEGATE_WIN_H_
