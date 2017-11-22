// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_LIFECYCLE_UNIT_SINK_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_LIFECYCLE_UNIT_SINK_H_

namespace resource_coordinator {

class LifecycleUnit;

// Interface to be notified when LifecycleUnits are created and destroyed.
class LifecycleUnitSink {
 public:
  virtual ~LifecycleUnitSink() = default;

  // Invoked immediately after a LifecycleUnit is created.
  virtual void OnLifecycleUnitCreated(LifecycleUnit* lifecycle_unit) = 0;

  // Invoked just before a LifecycleUnit is destroyed.
  virtual void OnLifecycleUnitDestroyed(LifecycleUnit* lifecycle_unit) = 0;
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_LIFECYCLE_UNIT_SINK_H_
