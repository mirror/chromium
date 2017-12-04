// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_LIFECYCLE_UNIT_SOURCE_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_LIFECYCLE_UNIT_SOURCE_H_

#include "base/macros.h"
#include "base/observer_list.h"

namespace resource_coordinator {

class LifecycleUnit;
class LifecycleUnitSourceObserver;

// Base class for a class that creates and destroys LifecycleUnits.
class LifecycleUnitSource {
 public:
  LifecycleUnitSource();
  virtual ~LifecycleUnitSource();

  // Adds / removes an observer that is notified when a LifecycleUnit is created
  // or destroyed by this LifecycleUnitSource.
  void AddObserver(LifecycleUnitSourceObserver* observer);
  void RemoveObserver(LifecycleUnitSourceObserver* observer);

 protected:
  // Notifies observers that a LifecycleUnit was created / destroyed.
  void NotifyLifecycleUnitCreated(LifecycleUnit* lifecycle_unit);
  void NotifyLifecycleUnitDestroyed(LifecycleUnit* lifecycle_unit);

 private:
  // Observers notified when a LifecycleUnit is created or destroyed.
  base::ObserverList<LifecycleUnitSourceObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(LifecycleUnitSource);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_LIFECYCLE_UNIT_SOURCE_H_
