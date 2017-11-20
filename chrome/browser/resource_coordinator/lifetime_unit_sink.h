// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_LIFETIME_UNIT_SINK_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_LIFETIME_UNIT_SINK_H_

namespace resource_coordinator {

class LifetimeUnit;

// Interface to be notified when LifetimeUnits are created and destroyed.
class LifetimeUnitSink {
 public:
  virtual ~LifetimeUnitSink() = default;

  // Invoked immediately after a LifetimeUnit is created.
  virtual void OnLifetimeUnitCreated(LifetimeUnit* lifetime_unit) = 0;

  // Invoked just before a LifetimeUnit is destroyed.
  virtual void OnLifetimeUnitDestroyed(LifetimeUnit* lifetime_unit) = 0;
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_LIFETIME_UNIT_SINK_H_
