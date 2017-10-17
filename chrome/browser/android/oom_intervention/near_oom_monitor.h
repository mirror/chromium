// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_OOM_INTERVENTION_NEAR_OOM_MONITOR_H_
#define CHROME_BROWSER_ANDROID_OOM_INTERVENTION_NEAR_OOM_MONITOR_H_

#include "base/observer_list_threadsafe.h"
#include "base/time/time.h"
#include "base/timer/timer.h"

class NearOomMonitor {
 public:
  class Observer {
   public:
    virtual ~Observer() {}
    virtual void OnNearOomDetected() = 0;
  };

  static NearOomMonitor* GetInstance();

  NearOomMonitor();
  ~NearOomMonitor();

  void Start();

  void Register(Observer* observer);
  void Unregister(Observer* observer);

 private:
  void Check();

  base::TimeDelta monitoring_interval_;
  base::RepeatingTimer timer_;

  int64_t swapfree_threshold_;

  scoped_refptr<base::ObserverListThreadSafe<Observer>> observers_;

  DISALLOW_COPY_AND_ASSIGN(NearOomMonitor);
};

#endif  // CHROME_BROWSER_ANDROID_OOM_INTERVENTION_NEAR_OOM_MONITOR_H_
