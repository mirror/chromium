// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_LIFETIME_UNIT_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_LIFETIME_UNIT_H_

#include "base/containers/flat_set.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "chrome/browser/resource_coordinator/discard_condition.h"

namespace resource_coordinator {

// A LifetimeUnit represents a unit that can switch between the "alive" and
// "discarded" states. When it is alive, the unit uses system resources and
// provides functionality to the user. When it is discarded, the unit doesn't
// use any system resource.
class LifetimeUnit {
 public:
  enum class State {
    // The LifetimeUnit is using system resources.
    ALIVE,
    // The LifetimeUnit is not using system resources.
    DISCARDED,
  };

  // Used to sort LifetimeUnit by importance. The most important LifetimeUnit
  // has the greatest SortKey.
  struct SortKey {
    SortKey();
    explicit SortKey(base::TimeTicks last_focused_time);

    bool operator<(const SortKey& other) const;
    bool operator>(const SortKey& other) const;

    // Last time at which the LifetimeUnit was focused. base::TimeTicks::Max()
    // if the LifetimeUnit is currently focused.
    base::TimeTicks last_focused_time;
  };

  virtual ~LifetimeUnit() = default;

  // Returns a title describing this LifetimeUnit.
  virtual base::string16 GetTitle() const = 0;

  // Returns a key that can be used to evaluate the relative importance of this
  // LifetimeUnit.
  virtual SortKey GetSortKey() const = 0;

  // Returns the current state of this LifetimeUnit.
  virtual State GetState() const = 0;

  // Returns the estimated number of kilobytes that would be freed if this
  // LifetimeUnit was discarded.
  virtual int GetEstimatedMemoryFreedOnDiscardKB() const = 0;

  // Returns true if this LifetimeUnit can be discared.
  virtual bool CanDiscard(DiscardCondition discard_condition) const = 0;

  // Discards this LifetimeUnit.
  virtual bool Discard(DiscardCondition discard_condition) = 0;
};

using LifetimeUnitSet = base::flat_set<LifetimeUnit*>;

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_LIFETIME_UNIT_H_
