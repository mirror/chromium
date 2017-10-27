// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The Windows implementation of the delegate used by the swap thrashing
// monitor.
//
// The systems interested in observing the thrashing activity should ensure that
// the SampleAndCalculateSwapThrashingLevel function of this delegate gets
// called periodically as it is responsible for querying the state of the
// system. It is recommended to use a frequency around 0.5 and 1Hz.
//
// This delegate looks at a fixed number of past samples to determine if the
// system is now in a thrashing state. To do this it counts how many samples
// exceed a certain threshold.

#ifndef CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_WIN_H_
#define CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_WIN_H_

#include <list>

#include "base/containers/circular_deque.h"
#include "base/optional.h"
#include "base/sequence_checker.h"
#include "chrome/browser/memory/swap_thrashing_level.h"

namespace memory {

class SwapThrashingMonitorDelegateWin {
 public:
  SwapThrashingMonitorDelegateWin();
  virtual ~SwapThrashingMonitorDelegateWin();

  // Calculates the swap-thrashing level over the interval between now and the
  // last time this function was called. This function will always return
  // SWAP_THRASHING_LEVEL_NONE when it gets called for the first time.
  //
  // This function requires sequence-affinity, through use of ThreadChecker. It
  // is also blocking and should be run on a blocking sequenced task runner.
  //
  // TODO(sebmarchand): Check if this should be posted to a task runner and run
  // asynchronously.
  SwapThrashingLevel SampleAndCalculateSwapThrashingLevel();

 protected:
  // Used to compute how many of the recent samples are above a given threshold.
  class SampleWindow {
   public:
    SampleWindow();

    // Virtual for unittesting.
    virtual ~SampleWindow();

    // Should be called when a new hard-page fault observation is made. The
    // delta with the previous observation will be added to the list of deltas.
    void OnObservation(uint64_t sample);

    size_t observation_abobe_threshold() {
      return observation_abobe_threshold_;
    }

   protected:
    // The number of sample that this window should keep track of.
    const size_t kSampleCount;

    // The last observation that has been made.
    base::Optional<uint64_t> last_observation_;

    // The delta between each observation.
    base::circular_deque<size_t> observation_deltas_;

    // The number of observations that are above the high swapping threshold.
    size_t observation_abobe_threshold_;

   private:
    SEQUENCE_CHECKER(sequence_checker_);

    DISALLOW_COPY_AND_ASSIGN(SampleWindow);
  };

  // Record the sum of the hard page-fault count for all the Chrome processes.
  //
  // Virtual for unittesting.
  virtual bool RecordHardFaultCountForChromeProcesses();

  std::unique_ptr<SampleWindow> samples_window_;

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SwapThrashingMonitorDelegateWin);
};

}  // namespace memory

#endif  // CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_WIN_H_
