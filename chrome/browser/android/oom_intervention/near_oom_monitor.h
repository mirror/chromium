// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_OOM_INTERVENTION_NEAR_OOM_MONITOR_H_
#define CHROME_BROWSER_ANDROID_OOM_INTERVENTION_NEAR_OOM_MONITOR_H_

#include "base/observer_list.h"
#include "base/process/process_metrics.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/android/oom_intervention/near_oom_observer.h"

// NearOomMonitor tracks memory stats to estimate whether we are in "near-OOM"
// situation. Near-OOM is defined as a situation where the foreground apps
// will be killed soon when we keep allocating memory.
// This monitor periodically checks memory stats and notifies observers when
// the monitor detects near-OOM situation.
class NearOomMonitor {
 public:
  NearOomMonitor();
  virtual ~NearOomMonitor();

  // Starts monitoring. Returns true when monitoring is actually started.
  bool Start();

  base::TimeDelta GetMonitoringInterval() const { return monitoring_interval_; }

  bool IsRunning() const;

  // Registers/unregisters an observer. NearOomMonitor doesn't take the
  // ownership of observers. These methods aren't thread-safe.
  // OnNearOomDetected() will be executed on the task runner on which this
  // monitor is running.
  void Register(NearOomObserver* observer);
  void Unregister(NearOomObserver* observer);

 protected:
  // Constructs with a TaskRunner. Used for tests.
  explicit NearOomMonitor(scoped_refptr<base::SequencedTaskRunner> task_runner);

  // Gets system memory info. This is a virtual method so that we can override
  // this for testing.
  virtual bool GetSystemMemoryInfo(base::SystemMemoryInfoKB* memory_info);

 private:
  // Checks whether we are in near-OOM situation. Called every
  // |monitoring_interval_|.
  void Check();

  base::TimeDelta monitoring_interval_;
  base::RepeatingTimer timer_;

  int64_t swapfree_threshold_;

  using NearOomObserverList = base::ObserverList<NearOomObserver>;
  NearOomObserverList observers_;

  DISALLOW_COPY_AND_ASSIGN(NearOomMonitor);
};

#endif  // CHROME_BROWSER_ANDROID_OOM_INTERVENTION_NEAR_OOM_MONITOR_H_
