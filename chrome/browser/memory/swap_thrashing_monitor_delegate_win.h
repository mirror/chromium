// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// SwapThrashingMonitorDelegateWin defines a state-based swap thrashing monitor
// on Windows. Thrashing is continuous swap activity, caused by processes
// needing to touch more pages than fit in physical memory, over a given period
// of time. DO NOT SUBMIT The systems interested in observing these signals
// should ensure that the SampleAndCalculateSwapThrashingLevel function of this
// detector gets called periodically as it is responsible for querying the state
// of the system.
//
// This monitor defines the following state and transitions:
//
//       ┎─────────────────────────┐
//       ┃SWAP_THRASHING_LEVEL_NONE┃
//       ┖─────────────────────────┚
//           |              ▲
//           | (T1)         | (T1')
//           ▼              |
//    ┎──────────────────────────────┒
//    ┃SWAP_THRASHING_LEVEL_SUSPECTED┃
//    ┖──────────────────────────────┚
//           |              ▲
//           | (T2)         | (T2')
//           ▼              |
//    ┎──────────────────────────────┒
//    ┃SWAP_THRASHING_LEVEL_CONFIRMED┃
//    ┖──────────────────────────────┚
//
// SWAP_THRASHING_LEVEL_NONE is the initial level, it means that there's no
// thrashing happening on the system or that it is undetermined. The monitor
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
// From the SWAP_THRASHING_LEVEL_CONFIRMED level the monitor can only go back
// to the SWAP_THRASHING_LEVEL_SUSPECTED state if there's no swap-thrashing
// observed during an interval of time T2' (T2 with an hysteresis).

#ifndef CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_WIN_H_
#define CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_WIN_H_

#include <list>

#include "base/containers/circular_deque.h"
#include "base/optional.h"
#include "base/sequence_checker.h"
#include "chrome/browser/memory/swap_thrashing_common.h"

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
    // |sample_count| is the maximum number of samples that this window will
    // keep track off.
    explicit SampleWindow(const size_t sample_count);
    virtual ~SampleWindow();

    // Should be called when a new hard-page fault observation is made. The
    // delta with the previous observation will be added to the list of deltas.
    virtual void OnObservation(uint64_t sample);

    // Returns the number of samples that are above a given threshold;
    size_t NumberOfSamplesAboveThreshold(const size_t threshold) const;

   protected:
    // The number of sample that this window should keep track of.
    const size_t kSampleCount;

    // The last observation that has been made.
    uint64_t last_observation_;

    // Indicates if the last observation is valid.
    bool last_observation_valid_;

    // The delta between each observation.
    base::circular_deque<uint64_t> observation_deltas_;

   private:
    SEQUENCE_CHECKER(sequence_checker_);

    DISALLOW_COPY_AND_ASSIGN(SampleWindow);
  };

  // Record the sum of the hard page-fault count for all the Chrome processes.
  virtual bool RecordHardFaultCountForChromeProcesses();

  std::unique_ptr<SampleWindow> samples_window_;

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SwapThrashingMonitorDelegateWin);
};

}  // namespace memory

#endif  // CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_WIN_H_
