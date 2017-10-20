// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_OOM_INTERVENTION_NEAR_OOM_OBSERVER_H_
#define CHROME_BROWSER_ANDROID_OOM_INTERVENTION_NEAR_OOM_OBSERVER_H_

// An interface to observe near-OOM situation. See NearOomMonitor for details.
class NearOomObserver {
 public:
  // Called when NearOomMonitor detects near-OOM situation.
  virtual void OnNearOomDetected() = 0;

 protected:
  virtual ~NearOomObserver() = default;
};

#endif  // CHROME_BROWSER_ANDROID_OOM_INTERVENTION_NEAR_OOM_OBSERVER_H_
